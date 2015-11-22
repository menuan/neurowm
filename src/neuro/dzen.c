//----------------------------------------------------------------------------------------------------------------------
// Module      :  dzen
// Copyright   :  (c) Julian Bouzas 2014
// License     :  BSD3-style (see LICENSE)
// Maintainer  :  Julian Bouzas - nnoell3[at]gmail.com
// Stability   :  stable
//----------------------------------------------------------------------------------------------------------------------


//----------------------------------------------------------------------------------------------------------------------
// PREPROCESSOR
//----------------------------------------------------------------------------------------------------------------------

// Includes
#include "dzen.h"
#include "system.h"
#include "core.h"

// Defines
#define CPU_FILE_PATH "/proc/stat"
#define CPU_MAX_VALS 10


//----------------------------------------------------------------------------------------------------------------------
// PRIVATE VARIABLE DECLARATION
//----------------------------------------------------------------------------------------------------------------------

// CPU Calculation
typedef struct CpuInfo CpuInfo;
struct CpuInfo {
  long idle;
  long total;
  int perc;
};

typedef struct CpuCalcRefreshInfo CpuCalcRefreshInfo;
struct CpuCalcRefreshInfo {
  pthread_t thread;
  pthread_mutex_t mutex;
  pthread_cond_t cond;
  CpuInfo *cpu_info;
  int num_cpus;
};

// Dzen
typedef struct PipeInfo PipeInfo;
struct PipeInfo {
  int output;
  pid_t pid;
};

typedef struct DzenRefreshInfo DzenRefreshInfo;
struct DzenRefreshInfo {
  pthread_t thread;
  pthread_mutex_t mutex;
  pthread_cond_t cond;
  PipeInfo *pipe_info;
  int num_panels;
  int reset_rate;
};


//----------------------------------------------------------------------------------------------------------------------
// PRIVATE VARIABLE DEFINITION
//----------------------------------------------------------------------------------------------------------------------

// CPU Calculation
static CpuCalcRefreshInfo cpu_calc_refresh_info_;
static Bool cpu_stop_refresh_cond_ = False;

// Dzen
static DzenRefreshInfo dzen_refresh_info_;
static Bool dzen_stop_refresh_cond_ = False;


//----------------------------------------------------------------------------------------------------------------------
// PRIVATE FUNCTION DEFINITION
//----------------------------------------------------------------------------------------------------------------------

// CPU Calculation
static int get_num_cpus(const char *file) {
  assert(file);
  FILE *fd = fopen(file, "r");
  if (fd == NULL)
    return 0;
  int i = 0;
  char buf[ 256 ];
  while (fgets(buf, sizeof(buf), fd)) {
    if (strncmp(buf, "cpu", 3) != 0)
      break;
    ++i;
  }
  fclose(fd);
  return i;
}

static void get_perc_info(CpuInfo *cpu_info, long *cpu_vals, long prev_idle, long prev_total) {
  assert(cpu_info);
  assert(cpu_vals);
  cpu_info->idle = cpu_vals[ 3 ];
  cpu_info->total = 0L;
  int i;
  for (i = 0; i < CPU_MAX_VALS; ++i)
    cpu_info->total += cpu_vals[ i ];
  long diffIdle = cpu_info->idle - prev_idle;
  long diffTotal = cpu_info->total - prev_total;
  cpu_info->perc = (100 * (diffTotal - diffIdle)) / diffTotal;
}

// Returns True if it has timed out and false when the condition variable has been notified
static Bool refresh_cpu_calc_timedwait(int seconds) {
  struct timespec deadline;
  clock_gettime(CLOCK_REALTIME, &deadline);
  deadline.tv_sec += seconds;
  Bool timedout = False;
  pthread_mutex_lock(&cpu_calc_refresh_info_.mutex);
  while (!cpu_stop_refresh_cond_) {
    int res = pthread_cond_timedwait(&cpu_calc_refresh_info_.cond, &cpu_calc_refresh_info_.mutex, &deadline);
    if (ETIMEDOUT == res) {
      timedout = True;
      break;
    }
  }
  pthread_mutex_unlock(&cpu_calc_refresh_info_.mutex);
  return timedout;
}

static void refresh_cpu_calc(const char *file, int ncpus) {
  assert(file);
  long cpusFileInfo[ ncpus ][ CPU_MAX_VALS ];
  long prevIdle[ ncpus ], prevTotal[ ncpus ];
  memset(prevIdle, 0, sizeof(prevIdle));
  memset(prevTotal, 0, sizeof(prevTotal));

  while (True) {
    // Open the file
    FILE *fd = fopen(file, "r");
    if (fd == NULL)
      return;

    // Do the percent clalculation
    int i;
    char buf[ 256 ];
    for (i = 0; i < ncpus; ++i) {
      fgets(buf, sizeof(buf), fd);
      if (EOF == sscanf(buf + 5, "%li %li %li %li %li %li %li %li %li %li",
          cpusFileInfo[ i ] + 0, cpusFileInfo[ i ] + 1,
          cpusFileInfo[ i ] + 2, cpusFileInfo[ i ] + 3,
          cpusFileInfo[ i ] + 4, cpusFileInfo[ i ] + 5,
          cpusFileInfo[ i ] + 6, cpusFileInfo[ i ] + 7,
          cpusFileInfo[ i ] + 8, cpusFileInfo[ i ] + 9))
        return;
      get_perc_info(cpu_calc_refresh_info_.cpu_info + i, cpusFileInfo[ i ], prevIdle[ i ], prevTotal[ i ]);
      prevIdle[ i ] = cpu_calc_refresh_info_.cpu_info[ i ].idle;
      prevTotal[ i ] = cpu_calc_refresh_info_.cpu_info[ i ].total;
    }

    // Close the file
    fclose(fd);

    // Wait 1 second or break if the conditional variable has been signaled
    if (!refresh_cpu_calc_timedwait(1))
      break;
  }
}

static void *refresh_cpu_calc_thread(void *args) {
  (void)args;
  refresh_cpu_calc(CPU_FILE_PATH, cpu_calc_refresh_info_.num_cpus);
  pthread_exit(NULL);
}

static Bool init_cpu_calc_thread() {
  // Init mutex and cond
  pthread_mutex_init(&cpu_calc_refresh_info_.mutex, NULL);
  pthread_cond_init(&cpu_calc_refresh_info_.cond, NULL);

  // Create thread
  return pthread_create(&cpu_calc_refresh_info_.thread, NULL, refresh_cpu_calc_thread, NULL);
}

static void stop_cpu_calc_thread() {
  // Stop calc thread
  cpu_stop_refresh_cond_ = True;
  pthread_cond_broadcast(&cpu_calc_refresh_info_.cond);

  // Join thread
  void *status;
  if (pthread_join(cpu_calc_refresh_info_.thread, &status))  // Wait
    perror("stop_cpu_perc_thread - Could not join thread");

  // Destroy cond and mutex
  pthread_cond_destroy(&cpu_calc_refresh_info_.cond);
  pthread_mutex_destroy(&cpu_calc_refresh_info_.mutex);
}

static Bool init_cpu_calc_refresh_info() {
  cpu_calc_refresh_info_.num_cpus = get_num_cpus(CPU_FILE_PATH);
  if (cpu_calc_refresh_info_.num_cpus <= 0)
    return False;
  cpu_calc_refresh_info_.cpu_info = (CpuInfo *)calloc(cpu_calc_refresh_info_.num_cpus, sizeof(CpuInfo));
  return cpu_calc_refresh_info_.cpu_info != NULL;
}

static void stop_cpu_calc_refresh_info() {
  free(cpu_calc_refresh_info_.cpu_info);
  cpu_calc_refresh_info_.cpu_info = NULL;
}

// Dzen
static char **str_to_cmd(char **cmd, char *str, const char *sep) {
  assert(cmd);
  assert(str);
  assert(sep);
  char *token, *saveptr;
  token = strtok_r(str, sep, &saveptr);
  int i;
  for (i = 0; token; ++i) {
    cmd[ i ] = token;
    token = strtok_r(NULL, sep, &saveptr);
  }
  cmd[ i ] = NULL;
  return cmd;
}

static char **get_dzen_cmd(char **cmd, char *line, const DzenFlags *df) {
  assert(cmd);
  assert(line);
  assert(df);
  snprintf(line, DZEN_LINE_MAX, "/usr/bin/dzen2 -x %i -y %i -w %i -h %i -fg %s -bg %s -ta %c -fn %s -e %s %s",
      df->x, df->y, df->w, df->h, df->fgColor, df->bgColor, df->align, df->font, df->event, df->extras);
  return str_to_cmd(cmd, line, " \t\n");
}

// Returns True if it has timed out and false when the condition variable has been notified
static Bool refresh_dzen_panel_timedwait(int seconds) {
  struct timespec deadline;
  clock_gettime(CLOCK_REALTIME, &deadline);
  deadline.tv_sec += seconds;
  Bool timedout = False;
  pthread_mutex_lock(&dzen_refresh_info_.mutex);
  while (!dzen_stop_refresh_cond_) {
    int res = pthread_cond_timedwait(&dzen_refresh_info_.cond, &dzen_refresh_info_.mutex, &deadline);
    if (ETIMEDOUT == res) {
      timedout = True;
      break;
    }
  }
  pthread_mutex_unlock(&dzen_refresh_info_.mutex);
  return timedout;
}

static void refresh_dzen_panel(const DzenPanel *dp, int fd) {
  assert(dp);
  char line[ DZEN_LINE_MAX ] = "\0";
  int i;
  for (i = 0; dp->loggers[ i ]; ++i) {
    char str[ LOGGER_MAX ] = "\0";
    dp->loggers[ i ](str);
    if (i > 0 && str[ 0 ] != '\0')  // Add separator if not first and not empty str
      strncat(line, dp->sep, DZEN_LINE_MAX - strlen(line) - 1);
    strncat(line, str, DZEN_LINE_MAX - strlen(line) - 1);
  }
  strncat(line, "\n", DZEN_LINE_MAX - strlen(line) - 1);  // Line must be '\n' terminated so that dzen shows it
  write(fd, line, strlen(line));
}

static void *refresh_dzen_panel_thread(void *args) {
  (void)args;
  const DzenPanel *dp;
  int i = 0, j;
  while (True) {
    // Update all panels respecting their refresh rate
    for (j = 0; j < dzen_refresh_info_.num_panels; ++j) {
      dp = NeuroSystemGetConfiguration()->dzenPanelSet[ j ];
      if (dp->refreshRate == WM_EVENT || dp->refreshRate <= 0)
        continue;
      if (i % dp->refreshRate == 0)
        refresh_dzen_panel(dp, dzen_refresh_info_.pipe_info[ j ].output);
    }
    ++i;
    i %= dzen_refresh_info_.reset_rate;

    // Wait 1 second or break if the conditional variable has been signaled
    if (!refresh_dzen_panel_timedwait(1))
      break;
  }
  pthread_exit(NULL);
}

static Bool init_dzen_refresh_thread() {
  if (dzen_refresh_info_.reset_rate <= 0)
    return True;

  // Init mutex and cond
  pthread_mutex_init(&dzen_refresh_info_.mutex, NULL);
  pthread_cond_init(&dzen_refresh_info_.cond, NULL);

  // Create thread
  return pthread_create(&dzen_refresh_info_.thread, NULL, refresh_dzen_panel_thread, NULL) == 0;
}

static void stop_dzen_refresh_thread() {
  if (dzen_refresh_info_.reset_rate <= 0)
    return;

  // Stop refresh thread
  dzen_stop_refresh_cond_ = True;
  pthread_cond_broadcast(&dzen_refresh_info_.cond);

  // Join thread
  void *status;
  if (pthread_join(dzen_refresh_info_.thread, &status))  // Wait
    perror("stop_refresh_thread - Could not join thread");

  // Init cond and mutex
  pthread_cond_destroy(&dzen_refresh_info_.cond);
  pthread_mutex_destroy(&dzen_refresh_info_.mutex);
}

static Bool init_dzen_refresh_info() {
  const DzenPanel *const *const confPanelSet = NeuroSystemGetConfiguration()->dzenPanelSet;
  dzen_refresh_info_.num_panels = NeuroTypeArrayLength((const void const *const *)confPanelSet);
  dzen_refresh_info_.pipe_info = (PipeInfo *)calloc(dzen_refresh_info_.num_panels, sizeof(PipeInfo));
  if (!dzen_refresh_info_.pipe_info)
    return False;
  dzen_refresh_info_.reset_rate = 1;
  const DzenPanel *dp;
  int i;
  for (i = 0; i < dzen_refresh_info_.num_panels; ++i) {
    dp = confPanelSet[ i ];

    // Get max refresh rate
    if (dp->refreshRate > dzen_refresh_info_.reset_rate)
      dzen_refresh_info_.reset_rate *= dp->refreshRate;

    // Create a dzen pipe for every panel
    char *dzen_cmd[ DZEN_ARGS_MAX ];
    char line[ DZEN_LINE_MAX ];
    get_dzen_cmd(dzen_cmd, line, dp->df);
    dzen_refresh_info_.pipe_info[ i ].output = NeuroSystemSpawnPipe((const char *const *)dzen_cmd,
        &(dzen_refresh_info_.pipe_info[ i ].pid));
    if (dzen_refresh_info_.pipe_info[ i ].output == -1)
      return False;
  }
  return True;
}

static void stop_dzen_refresh_info() {
  int i;
  for (i = 0; i < dzen_refresh_info_.num_panels; ++i)
    if (kill(dzen_refresh_info_.pipe_info[ i ].pid, SIGTERM) == -1)
      perror("stop_dzen_pipe_info_panels - Could not kill panels");
  free(dzen_refresh_info_.pipe_info);
  dzen_refresh_info_.pipe_info = NULL;
}


//----------------------------------------------------------------------------------------------------------------------
// PUBLIC FUNCTION DEFINITION
//----------------------------------------------------------------------------------------------------------------------

// Dzen
Bool NeuroDzenInit() {
  if (!init_dzen_refresh_info() || !init_dzen_refresh_thread())
    return False;
  NeuroDzenRefresh(False);
  return True;
}

void NeuroDzenStop() {
  stop_dzen_refresh_thread();
  stop_dzen_refresh_info();
}

void NeuroDzenRefresh(Bool on_event_only) {
  const DzenPanel *dp;
  int i;
  for (i=0; i < dzen_refresh_info_.num_panels; ++i) {
    dp = NeuroSystemGetConfiguration()->dzenPanelSet[ i ];
    if (on_event_only) {
      if (dp->refreshRate == WM_EVENT || dp->refreshRate <= 0)
        refresh_dzen_panel(dp, dzen_refresh_info_.pipe_info[ i ].output);
    } else {
      refresh_dzen_panel(dp, dzen_refresh_info_.pipe_info[ i ].output);
    }
  }
}

void NeuroDzenInitCpuCalc() {
  if (!init_cpu_calc_refresh_info())
    NeuroSystemError("NeuroDzenInitCpuCalc - Could not init CPU Set");
  if (init_cpu_calc_thread())
    NeuroSystemError("NeuroDzenInitCpuCalc - Could not init CPU percent thread");
}

void NeuroDzenStopCpuCalc() {
  stop_cpu_calc_thread();
  stop_cpu_calc_refresh_info();
}

void NeuroDzenWrapDzenBox(char *dst, const char *src, const BoxPP *b) {
  assert(dst);
  assert(src);
  assert(b);
  snprintf(dst, LOGGER_MAX,
      "^fg(%s)^i(%s)^ib(1)^r(1920x%i)^p(-1920x)^fg(%s)%s^fg(%s)^i(%s)^fg(%s)^r(1920x%i)^p(-1920)^fg()^ib(0)",
      b->boxColor, b->leftIcon, b->boxHeight, b->fgColor, src, b->boxColor, b->rightIcon, b->bgColor, b->boxHeight);
}

void NeuroDzenWrapClickArea(char *dst, const char *src, const CA *ca) {
  assert(dst);
  assert(src);
  assert(ca);
  snprintf(dst, LOGGER_MAX, "^ca(1,%s)^ca(2,%s)^ca(3,%s)^ca(4,%s)^ca(5,%s)%s^ca()^ca()^ca()^ca()^ca()",
      ca->leftClick, ca->middleClick, ca->rightClick, ca->wheelUp, ca->wheelDown, src);
}

Bool NeuroDzenReadFirstLineFile(char *buf, const char *path) {
  assert(buf);
  assert(path);
  FILE *fd;
  fd = fopen(path, "r");
  if (!fd)
    return False;
  fgets(buf, LOGGER_MAX, fd);
  char *last = buf + strlen(buf) - 1;
  if (*last == '\n')
    *last = '\0';
  fclose(fd);
  return True;
}

// Loggers
void NeuroDzenLoggerTime(char *str) {
  assert(str);
  struct tm res;
  time_t t = time(NULL);
  localtime_r(&t, &res);
  snprintf(str, LOGGER_MAX, "%02d:%02d:%02d", res.tm_hour, res.tm_min, res.tm_sec);
}


void NeuroDzenLoggerDate(char *str) {
  assert(str);
  struct tm res;
  time_t t = time(NULL);
  localtime_r(&t, &res);
  snprintf(str, LOGGER_MAX, "%d.%02d.%02d", res.tm_year + 1900, res.tm_mon + 1, res.tm_mday);
}

void NeuroDzenLoggerDay(char *str) {
  assert(str);
  struct tm res;
  time_t t = time(NULL);
  localtime_r(&t, &res);
  switch (res.tm_wday) {
    case 1:  strncpy(str, "Monday", LOGGER_MAX); break;
    case 2:  strncpy(str, "Tuesday", LOGGER_MAX); break;
    case 3:  strncpy(str, "Wednesday", LOGGER_MAX); break;
    case 4:  strncpy(str, "Thursday", LOGGER_MAX); break;
    case 5:  strncpy(str, "Friday", LOGGER_MAX); break;
    case 6:  strncpy(str, "Saturday", LOGGER_MAX); break;
    case 0:  strncpy(str, "Sunday", LOGGER_MAX); break;
    default: strncpy(str, "Unknown", LOGGER_MAX); break;
  }
}

void NeuroDzenLoggerUptime(char *str) {
  assert(str);
  struct sysinfo info;
  sysinfo(&info);
  int hours = (int)(info.uptime / 3600UL);
  int hrest = (int)(info.uptime % 3600UL);
  int minutes = hrest / 60;
  int seconds = hrest % 60;
  snprintf(str, LOGGER_MAX, "%ih %im %is", hours, minutes, seconds);
}

void NeuroDzenLoggerCpu(char *str) {
  assert(str);
  char buf[ LOGGER_MAX ];
  int i;
  for (i = 0; i < cpu_calc_refresh_info_.num_cpus; ++i) {
    snprintf(buf, LOGGER_MAX, "%i%% ", cpu_calc_refresh_info_.cpu_info[ i ].perc);
    strncat(str, buf, LOGGER_MAX - strlen(str) - 1);
  }
  str[ strlen(str) - 1 ] = '\0';
}

void NeuroDzenLoggerRam(char *str) {
  assert(str);
  char buf[ LOGGER_MAX ];
  unsigned long memTotal = 0UL, memAvailable = 0UL;
  FILE *fd = fopen("/proc/meminfo", "r");
  if (!fd)
    return;
  fgets(buf, LOGGER_MAX, fd);
  sscanf(buf, "%*s %lu\n", &memTotal);
  fgets(buf, LOGGER_MAX, fd);
  fgets(buf, LOGGER_MAX, fd);
  sscanf(buf, "%*s %lu\n", &memAvailable);
  unsigned long memUsed = memTotal - memAvailable;
  int perc = (int)((memUsed * 100UL) / memTotal);
  snprintf(str, LOGGER_MAX, "%i%% %luMB", perc, memUsed / 1024UL);
  fclose(fd);
}

void NeuroDzenLoggerWifiStrength(char *str) {
  assert(str);
  char buf[ LOGGER_MAX ];
  FILE *fd = fopen("/proc/net/wireless", "r");
  if (!fd)
    return;
  int strength = 0, tmp = 0;
  fgets(buf, LOGGER_MAX, fd);
  fgets(buf, LOGGER_MAX, fd);
  fgets(buf, LOGGER_MAX, fd);
  sscanf(buf, "%*s %i   %i\n", &tmp, &strength);
  snprintf(str, LOGGER_MAX, "%i%%", strength);
  fclose(fd);
}

void NeuroDzenLoggerCurrWorkspace(char *str) {
  assert(str);
  const char *name = NeuroCoreStackGetName(NeuroCoreGetCurrStack());
  if (name)
    strncpy(str, name, LOGGER_MAX);
}

void NeuroDzenLoggerCurrLayout(char *str) {
  assert(str);
  int ws = NeuroCoreGetCurrStack();
  const LayoutConf *lc = NeuroCoreStackGetCurrLayoutConf(ws);
  if (lc)
    strncpy(str, lc->name, LOGGER_MAX);
}

void NeuroDzenLoggerCurrTitle(char *str) {
  assert(str);
  ClientPtrPtr c = NeuroCoreStackGetCurrClient(NeuroCoreGetCurrStack());
  if (c)
    strncpy(str, CLI_GET(c).title, LOGGER_MAX);
}

