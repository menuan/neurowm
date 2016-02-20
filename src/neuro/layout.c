//----------------------------------------------------------------------------------------------------------------------
// Module      :  layout
// Copyright   :  (c) Julian Bouzas 2014
// License     :  BSD3-style (see LICENSE)
// Maintainer  :  Julian Bouzas - nnoell3[at]gmail.com
// Stability   :  stable
//----------------------------------------------------------------------------------------------------------------------


//----------------------------------------------------------------------------------------------------------------------
// PREPROCESSOR
//----------------------------------------------------------------------------------------------------------------------

// Includes
#include "system.h"
#include "layout.h"
#include "geometry.h"
#include "core.h"
#include "workspace.h"
#include "rule.h"

// Defines
#define STEP_SIZE_REALLOC 32


//----------------------------------------------------------------------------------------------------------------------
// PRIVATE FUNCTION DEFINITION
//----------------------------------------------------------------------------------------------------------------------

static NeuroArrange *new_arrange(NeuroIndex ws, NeuroLayout *l) {
  if (!l)
    return NULL;

  // Allocate a new arrange
  NeuroArrange *const a = (NeuroArrange *)malloc(sizeof(NeuroArrange));
  if (!a)
    return NULL;

  // Set the region
  NeuroGeometryGetRelativeRectangle(&a->region, NeuroCoreStackGetRegion(ws), l->region);

  // Set the clients
  NeuroRectangle **rs = NULL, **frs = NULL;
  NeuroIndex i = 0U, size = 0U;
  for (NeuroClientPtrPtr c = NeuroCoreStackGetHeadClient(ws); c; c = NeuroCoreClientGetNext(c)) {
    // Skip free and fullscreen clients
    if (CLI_GET(c).free_setter_fn != NeuroRuleFreeSetterNull || CLI_GET(c).is_fullscreen)
      continue;

    // Update region if there are fixed clients
    if (CLI_GET(c).fixed_pos != NEURO_FIXED_POSITION_NULL) {
      NeuroRuleSetLayoutRegion(&a->region, c);
      continue;
    }

    // Realloc if memory is needed
    if (i >= size || i <= 0) {
      size += STEP_SIZE_REALLOC;
      rs = (NeuroRectangle **)realloc(rs, size*sizeof(void *));
      frs = (NeuroRectangle **)realloc(frs, size*sizeof(void *));
      if (!rs || !frs)
        return NULL;
    }

    // Set rs and frs arrays
    rs[ i ] = NeuroCoreClientGetRegion(c);
    frs[ i ] = &(CLI_GET(c).float_region);
    ++i;
  }

  // Complete the arrange
  a->size = i;
  a->client_regions = rs;
  a->client_float_regions = frs;
  a->parameters = l->parameters;
  return a;
}

static void delete_arrange(NeuroArrange *a) {
  if (!a)
    return;
  free(a->client_regions);
  a->client_regions = NULL;
  free(a->client_float_regions);
  a->client_float_regions = NULL;
  free(a);
  a = NULL;
}

static void get_best_positions_and_sizes(NeuroIndex n, int total, int *positions, int *sizes) {
  assert(positions);
  assert(sizes);
  assert(n > 0);
  int position = 0, size = 0;
  for (NeuroIndex i = 0U; i < n; ++i) {
    size = total / (n - i);
    positions[ i ] = position;
    sizes[ i ] = size;
    position += size;
    total -= size;
  }
}

// NeuroArrange runners
static NeuroArrange *normal_arrange(NeuroArrange *a, NeuroArrangerFn af) {
  assert(a);
  assert(af);
  af(a);
  return a;
}

static NeuroArrange *mirror_arrange(NeuroArrange *a, NeuroArrangerFn af) {
  assert(a);
  assert(af);
  NeuroGeometryTranspRectangle(&a->region);
  af(a);
  for (NeuroIndex i = 0U; i < a->size; ++i)
    NeuroGeometryTranspRectangle(a->client_regions[ i ]);
  NeuroGeometryTranspRectangle(&a->region);
  return a;
}

// Mods
static NeuroArrange *reflect_x_mod(NeuroArrange *a) {
  assert(a);
  for (NeuroIndex i = 0U; i < a->size; ++i)
    a->client_regions[ i ]->x = 2*(a->region.x) + a->region.w - (a->client_regions[ i ]->x + a->client_regions[ i ]->w);
  return a;
}

static NeuroArrange *reflect_y_mod(NeuroArrange *a) {
  assert(a);
  for (NeuroIndex i = 0U; i < a->size; ++i)
    a->client_regions[ i ]->y = 2*(a->region.y) + a->region.h - (a->client_regions[ i ]->y + a->client_regions[ i ]->h);
  return a;
}


//----------------------------------------------------------------------------------------------------------------------
// PUBLIC FUNCTION DEFINITION
//----------------------------------------------------------------------------------------------------------------------

void NeuroLayoutRun(NeuroIndex ws, NeuroIndex i) {
  NeuroLayout *const l = NeuroCoreStackGetLayout(ws, i);
  NeuroArrange *const a = new_arrange(ws, l);
  if (!a)
    NeuroSystemError("NeuroLayoutRun - Could not run layout");
  if (a->size) {  // Then run layout
    if (l->mod & NEURO_LAYOUT_MOD_MIRROR)
      mirror_arrange(a, l->arranger_fn);
    else
      normal_arrange(a, l->arranger_fn);
    if (l->mod & NEURO_LAYOUT_MOD_REFLECTX)
      reflect_x_mod(a);
    if (l->mod & NEURO_LAYOUT_MODE_REFLECTY)
      reflect_y_mod(a);
  }
  delete_arrange(a);
}

void NeuroLayoutRunCurr(NeuroIndex ws) {
  NeuroLayoutRun(ws, NeuroCoreStackGetLayoutIdx(ws));
}

void NeuroLayoutToggleMod(NeuroIndex ws, NeuroIndex i, NeuroLayoutMod mod) {
  NeuroLayout *const l = NeuroCoreStackGetLayout(ws, i);
  l->mod ^= mod;
  NeuroLayoutRun(ws, i);
  NeuroWorkspaceUpdate(ws);
}

void NeuroLayoutToggleModCurr(NeuroIndex ws, NeuroLayoutMod mod) {
  NeuroLayoutToggleMod(ws, NeuroCoreStackGetLayoutIdx(ws), mod);
}

void NeuroLayoutToggle(NeuroIndex ws, NeuroIndex i) {
  if (NeuroCoreStackIsCurrToggledLayout(ws))
    NeuroCoreStackSetToggledLayout(ws, NULL);
  else
    NeuroCoreStackSetToggledLayout(ws, &i);
  NeuroLayoutRunCurr(ws);
  NeuroWorkspaceFocus(ws);
}

void NeuroLayoutChange(NeuroIndex ws, int step) {
  const NeuroIndex i = NeuroCoreStackGetLayoutIdx(ws) + step;
  NeuroCoreStackSetLayoutIdx(ws, i);
  NeuroLayoutRunCurr(ws);
  NeuroWorkspaceFocus(ws);
}

void NeuroLayoutReset(NeuroIndex ws) {
  for (NeuroIndex i = 0U; i < NeuroCoreStackGetNumLayouts(ws); ++i) {
    NeuroLayout *const l = NeuroCoreStackGetLayout(ws, i);
    const NeuroLayoutConf *const lc = NeuroCoreStackGetLayoutConf(ws, i);
    l->mod = lc->mod;
    l->follow_mouse = lc->follow_mouse;
    memmove(l->parameters, lc->parameters, sizeof(NeuroArg)*ARRSET_MAX);
  }
  NeuroWorkspaceTile(ws);
  NeuroCoreStackSetLayoutIdx(ws, 0U);
  NeuroLayoutRunCurr(ws);
  NeuroWorkspaceFocus(ws);
}

void NeuroLayoutIncreaseMaster(NeuroIndex ws, int step) {
  NeuroArg *const as = NeuroCoreStackGetCurrLayout(ws)->parameters;
  const int res = as[ 0 ].int_ + step;
  if (res < 1)
    return;
  *(int *)&(as[ 0 ].int_) = res;
  NeuroLayoutRunCurr(ws);
  NeuroWorkspaceFocus(ws);
}

void NeuroLayoutResizeMaster(NeuroIndex ws, float factor) {
  const NeuroArg *const as = NeuroCoreStackGetCurrLayout(ws)->parameters;
  const float new_master_size = factor * as[ 2 ].float_ + as[ 1 ].float_;
  if (new_master_size <= 0.0f || new_master_size >= 1.0f)
    return;
  *(float *)&(as[ 1 ].float_) = new_master_size;
  NeuroLayoutRunCurr(ws);
  NeuroWorkspaceFocus(ws);
}

// NeuroLayout Arrangers
NeuroArrange *NeuroLayoutArrangerTall(NeuroArrange *a) {
  assert(a);
  const NeuroIndex n = a->size, mn = a->parameters[ 0 ].idx_, nwindows = n <= mn ? n : mn;
  const int ms = (int)(a->parameters[ 1 ].float_ * a->region.w);
  int ys[ n ], hs[ n ];
  memset(ys, 0, sizeof(ys));
  memset(hs, 0, sizeof(hs));
  get_best_positions_and_sizes(nwindows, a->region.h, ys, hs);
  NeuroIndex i = 0U;
  for ( ; i < nwindows; ++i)  // Master area
    NeuroGeometrySetRectangle(a->client_regions[ i ], a->region.x, a->region.y + ys[ i ],
        n > mn ? ms : a->region.w, hs[ i ]);
  if (n - nwindows == 0U)
    return a;
  get_best_positions_and_sizes(n - nwindows, a->region.h, ys, hs);
  for ( ; i < n; ++i)  // Stacking area
    NeuroGeometrySetRectangle(a->client_regions[ i ], a->region.x + ms, a->region.y + ys[ i-nwindows ],
        a->region.w - ms, hs[ i-nwindows ]);
  return a;
}

NeuroArrange *NeuroLayoutArrangerGrid(NeuroArrange *a) {
  assert(a);
  const NeuroIndex n = a->size;
  NeuroIndex cols = 0U;
  for ( ; cols <= n/2; ++cols)
    if (cols * cols >= n)
      break;
  NeuroIndex rows = n / cols;
  int xs[ cols ], ws[ cols ];
  memset(xs, 0, sizeof(xs));
  memset(ws, 0, sizeof(ws));
  get_best_positions_and_sizes(cols, a->region.w, xs, ws);
  NeuroIndex cn = 0U, rn = 0U;
  for (NeuroIndex i = 0U; i < n; ++i) {
    if (i/rows + 1 > cols - n%cols)
      rows = n/cols + 1;
    int ys[ rows ], hs[ rows ];
    memset(ys, 0, sizeof(ys));
    memset(hs, 0, sizeof(hs));
    get_best_positions_and_sizes(rows, a->region.h, ys, hs);
    NeuroGeometrySetRectangle(a->client_regions[ i ], a->region.x + xs[ cn ], a->region.y + ys[ rn ], ws[ cn ],
        hs[ rn ]);
    if (++rn >= rows) {
      rn = 0U;
      ++cn;
    }
  }
  return a;
}

NeuroArrange *NeuroLayoutArrangerFull(NeuroArrange *a) {
  assert(a);
  for (NeuroIndex i = 0U; i < a->size; ++i)
    NeuroGeometrySetRectangle(a->client_regions[ i ], a->region.x, a->region.y, a->region.w, a->region.h);
  return a;
}

NeuroArrange *NeuroLayoutArrangerFloat(NeuroArrange *a) {
  assert(a);
  for (NeuroIndex i = 0U; i < a->size; ++i) {
    const NeuroRectangle *const fr = a->client_float_regions[ i ];
    NeuroGeometrySetRectangle(a->client_regions[ i ], fr->x, fr->y, fr->w, fr->h);
    NeuroGeometryNeuroGeometryTranspRectangle(a->client_regions[ i ], &a->region);
  }
  return a;
}

