//----------------------------------------------------------------------------------------------------------------------
// Module      :  client
// Copyright   :  (c) Julian Bouzas 2014
// License     :  BSD3-style (see LICENSE)
// Maintainer  :  Julian Bouzas - nnoell3[at]gmail.com
// Stability   :  stable
//----------------------------------------------------------------------------------------------------------------------


//----------------------------------------------------------------------------------------------------------------------
// PREPROCESSOR
//----------------------------------------------------------------------------------------------------------------------

// Includes
#include "client.h"
#include "geometry.h"
#include "system.h"
#include "config.h"
#include "core.h"
#include "layout.h"
#include "rule.h"
#include "workspace.h"
#include "event.h"

//----------------------------------------------------------------------------------------------------------------------
// PRIVATE VARIABLE DECLARATION
//----------------------------------------------------------------------------------------------------------------------

// XMotionUpdaterFn
typedef void (*XMotionUpdaterFn)(NeuroRectangle *r, size_t ws, int cx, int cy, int cw, int ch, int ex, int ey, int px,
    int py);


//----------------------------------------------------------------------------------------------------------------------
// PRIVATE FUNCTION DEFINITION
//----------------------------------------------------------------------------------------------------------------------

static bool is_protocol_delete(Window w) {
  Atom *protocols;
  bool ret = false;
  int n = 0;
  if (XGetWMProtocols(NeuroSystemGetDisplay(), w, &protocols, &n)) {
    for (int i = 0; !ret && i < n; i++)
      if (protocols[ i ] == NeuroSystemGetWmAtom(NEURO_SYSTEM_WMATOM_DELETEWINDOW))
        ret = true;
    XFree(protocols);
  }
  return ret;
}

static bool set_title_atom(NeuroClient *c, Atom atom) {
  assert(c);
  XTextProperty tp;
  XGetTextProperty(NeuroSystemGetDisplay(), c->win, &tp, atom);
  if (!tp.nitems)
    return false;
  if (tp.encoding == XA_STRING) {
    strncpy(c->title, (char *)tp.value, NAME_MAX);
  } else {
    char **list = NULL;
    int n = 0;
    if (XmbTextPropertyToTextList(NeuroSystemGetDisplay(), &tp, &list, &n) >= Success && n > 0 && list[ 0 ]) {
      strncpy(c->title, list[ 0 ], NAME_MAX);
      XFreeStringList(list);
    }
  }
  XFree(tp.value);
  return true;
}

static NeuroClientPtrPtr query_pointer_client(size_t ws, int x, int y) {
  NeuroClientPtrPtr c = NeuroCoreStackGetHeadClient(ws);
  for ( ; c; c=NeuroCoreClientGetNext(c))
    if (NeuroGeometryIsPointInRectangle(NeuroCoreClientGetRegion(c), x, y))
      break;
  return c;
}

static void process_xmotion(NeuroRectangle *r, size_t ws, int cx, int cy, int cw, int ch, int px, int py,
    XMotionUpdaterFn mpf) {
  const bool res = XGrabPointer(NeuroSystemGetDisplay(), NeuroSystemGetRoot(), false,
      ButtonPressMask|ButtonReleaseMask|PointerMotionMask, GrabModeAsync, GrabModeAsync, None,
      NeuroSystemGetCursor(NEURO_SYSTEM_CURSOR_MOVE), CurrentTime);
  if (res != GrabSuccess)
    return;
  XEvent ev;
  do {
    XMaskEvent(NeuroSystemGetDisplay(), ButtonPressMask|ButtonReleaseMask|PointerMotionMask, &ev);
    if (ev.type == MotionNotify)
      mpf(r, ws, cx, cy, cw, ch, ev.xmotion.x, ev.xmotion.y, px, py);
  } while (ev.type != ButtonRelease);
  XUngrabPointer(NeuroSystemGetDisplay(), CurrentTime);
}

static void xmotion_move(NeuroRectangle *r, size_t ws, int cx, int cy, int cw, int ch, int ex, int ey, int px, int py) {
  (void)cw;
  (void)ch;
  r->x = cx + (ex - px);
  r->y = cy + (ey - py);
  NeuroLayoutRunCurr(ws);
  NeuroWorkspaceUpdate(ws);
}

static void xmotion_resize(NeuroRectangle *r, size_t ws, int cx, int cy, int cw, int ch, int ex, int ey, int px, int py) {
  (void)cx;
  (void)cy;
  (void)px;
  (void)py;
  r->w = cw + (ex - (cw + r->x));
  r->h = ch + (ey - (ch + r->y));
  NeuroLayoutRunCurr(ws);
  NeuroWorkspaceUpdate(ws);
}


//----------------------------------------------------------------------------------------------------------------------
// PUBLIC FUNCTION DEFINITION
//----------------------------------------------------------------------------------------------------------------------

// Basic Functions
void NeuroClientUpdate(NeuroClientPtrPtr c, const void *data) {
  (void)data;
  if (!c)
    return;

  // Get workspace and regions
  const size_t ws = CLI_GET(c).ws;
  const NeuroRectangle *const stack_region = NeuroCoreStackGetRegion(ws);
  NeuroRectangle *const client_region = NeuroCoreClientGetRegion(c);

  // Priority: Fullscreen > Free > Fixed > Tiled
  NeuroRectangle r = { 0 };
  if (CLI_GET(c).is_fullscreen) {
    NeuroGeometryUnsetRectangleGaps(&r, stack_region, NeuroCoreStackGetGaps(ws));
  } else if (CLI_GET(c).free_setter_fn != NeuroRuleFreeSetterNull) {
    CLI_GET(c).free_setter_fn(client_region, stack_region);
    memmove(&r, client_region, sizeof(NeuroRectangle));
  } else if (CLI_GET(c).fixed_pos != NEURO_FIXED_POSITION_NULL) {
    NeuroRuleSetClientRegion(&r, c);
  } else {
    memmove(&r, client_region, sizeof(NeuroRectangle));
  }

  // Set border width and border gap
  const NeuroLayout *const l = NeuroCoreStackGetCurrLayout(ws);
  const int border_width = l->border_width_setter_fn(c);
  const int border_gap = l->border_gap_setter_fn(c);
  NeuroGeometrySetRectangleBorderWidthAndGap(&r, border_width, border_gap);
  if (r.w < 1)
    r.w = 1;
  if (r.h < 1)
    r.h = 1;

  // Draw
  XSetWindowBorder(NeuroSystemGetDisplay(), CLI_GET(c).win, l->border_color_setter_fn(c));
  XSetWindowBorderWidth(NeuroSystemGetDisplay(), CLI_GET(c).win, border_width);
  XMoveResizeWindow(NeuroSystemGetDisplay(), CLI_GET(c).win, r.x, r.y, r.w, r.h);
}

void NeuroClientUpdateClassAndName(NeuroClientPtrPtr c, const void *data) {
  (void)data;
  if (!c)
    return;
  CLI_GET(c).class[ 0 ] = '\0';
  CLI_GET(c).name[ 0 ] = '\0';
  XClassHint ch = { 0 };
  if (!XGetClassHint(NeuroSystemGetDisplay(), CLI_GET(c).win, &ch))
    return;
  strncpy(CLI_GET(c).class, ch.res_class, NAME_MAX);
  strncpy(CLI_GET(c).name, ch.res_name, NAME_MAX);
  if (ch.res_class)
    XFree(ch.res_class);
  if (ch.res_name)
    XFree(ch.res_name);
}

void NeuroClientUpdateTitle(NeuroClientPtrPtr c, const void *data) {
  (void)data;
  if (!c)
    return;
  CLI_GET(c).title[ 0 ] = '\0';
  if (!set_title_atom(*c, NeuroSystemGetNetAtom(NEURO_SYSTEM_NETATOM_NAME)))
    set_title_atom(*c, XA_WM_NAME);
}

void NeuroClientSetUrgent(NeuroClientPtrPtr c, const void *data) {
  (void)data;
  if (!c)
    return;
  CLI_GET(c).is_urgent = true;
}

void NeuroClientUnsetUrgent(NeuroClientPtrPtr c, const void *data) {
  (void)data;
  if (!c)
    return;
  CLI_GET(c).is_urgent = false;
}

void NeuroClientKill(NeuroClientPtrPtr c, const void *data) {
  (void)data;
  if (!c)
    return;
  if (is_protocol_delete(CLI_GET(c).win)) {
    XEvent ke;
    ke.type = ClientMessage;
    ke.xclient.window = CLI_GET(c).win;
    ke.xclient.message_type = NeuroSystemGetWmAtom(NEURO_SYSTEM_WMATOM_PROTOCOLS);
    ke.xclient.format = 32;
    ke.xclient.data.l[ 0 ] = NeuroSystemGetWmAtom(NEURO_SYSTEM_WMATOM_DELETEWINDOW);
    ke.xclient.data.l[ 1 ] = CurrentTime;
    XSendEvent(NeuroSystemGetDisplay(), CLI_GET(c).win, false, NoEventMask, &ke);
  } else {
    XKillClient(NeuroSystemGetDisplay(), CLI_GET(c).win);
    NeuroEventUnmanageClient(c);
  }
}

void NeuroClientMinimize(NeuroClientPtrPtr c, const void *data) {
  (void)data;
  if (!c)
    return;
  NeuroCoreSetCurrClient(NeuroCoreClientGetNext(c));
  NeuroClient *cli = NeuroCoreRemoveClient(c);
  if (!cli)
    return;
  if (!NeuroCorePushMinimizedClient(cli))
    NeuroSystemError("NeuroClientMinimize - Could not minimize client");
  // Move client off screen
  XMoveWindow(NeuroSystemGetDisplay(), cli->win, NeuroSystemGetScreenRegion()->w + 1,
      NeuroSystemGetScreenRegion()->h + 1);
  NeuroLayoutRunCurr(cli->ws);
  NeuroWorkspaceFocus(cli->ws);
}

void NeuroClientTile(NeuroClientPtrPtr c, const void *data) {
  (void)data;
  if (!c)
    return;
  if (CLI_GET(c).free_setter_fn == NeuroRuleFreeSetterNull)
    return;
  CLI_GET(c).free_setter_fn = NeuroRuleFreeSetterNull;
  NeuroLayoutRunCurr(CLI_GET(c).ws);
  NeuroWorkspaceFocus(CLI_GET(c).ws);
}

void NeuroClientFree(NeuroClientPtrPtr c, const void *freeSetterFn) {
  if (!c)
    return;
  const NeuroArgFn *gaf = (const NeuroArgFn *)freeSetterFn;
  if (CLI_GET(c).free_setter_fn == gaf->FreeSetterFn_)
    return;
  CLI_GET(c).free_setter_fn = gaf->FreeSetterFn_;
  NeuroLayoutRunCurr(CLI_GET(c).ws);
  NeuroWorkspaceFocus(CLI_GET(c).ws);
}

void NeuroClientToggleFree(NeuroClientPtrPtr c, const void *free_setter_fn) {
  if (!c)
    return;
  if (CLI_GET(c).free_setter_fn != NeuroRuleFreeSetterNull)
    NeuroClientTile(c, NULL);
  else
    NeuroClientFree(c, free_setter_fn);
}

void NeuroClientNormal(NeuroClientPtrPtr c, const void *data) {
  (void)data;
  if (!c)
    return;
  if (!CLI_GET(c).is_fullscreen)
    return;
  CLI_GET(c).is_fullscreen = false;
  NeuroLayoutRunCurr(CLI_GET(c).ws);
  NeuroWorkspaceFocus(CLI_GET(c).ws);
}

void NeuroClientFullscreen(NeuroClientPtrPtr c, const void *data) {
  (void)data;
  if (!c)
    return;
  if (CLI_GET(c).is_fullscreen)
    return;
  CLI_GET(c).is_fullscreen = true;
  NeuroLayoutRunCurr(CLI_GET(c).ws);
  NeuroWorkspaceFocus(CLI_GET(c).ws);
}

void NeuroClientToggleFullscreen(NeuroClientPtrPtr c, const void *data) {
  (void)data;
  if (!c)
    return;
  if (CLI_GET(c).is_fullscreen)
    NeuroClientNormal(c, NULL);
  else
    NeuroClientFullscreen(c, NULL);
}

void NeuroClientFloatMove(NeuroClientPtrPtr c, const void *data) {
  (void)data;
  if (!c)
    return;
  NeuroWorkspaceClientFocus(c, NeuroClientSelectorSelf, NULL);
  if (CLI_GET(c).fixed_pos != NEURO_FIXED_POSITION_NULL)
    return;
  NeuroRectangle *r = &(CLI_GET(c).float_region);
  int px = 0, py = 0;
  NeuroClientGetPointed(&px, &py);
  process_xmotion(r, CLI_GET(c).ws, r->x, r->y, r->w, r->h, px, py, xmotion_move);
}

void NeuroClientFloatResize(NeuroClientPtrPtr c, const void *data) {
  (void)data;
  if (!c)
    return;
  NeuroWorkspaceClientFocus(c, NeuroClientSelectorSelf, NULL);
  if (CLI_GET(c).fixed_pos != NEURO_FIXED_POSITION_NULL)
    return;
  NeuroRectangle *r = &(CLI_GET(c).float_region);
  int px = 0, py = 0;
  NeuroClientGetPointed(&px, &py);
  process_xmotion(r, CLI_GET(c).ws, r->x, r->y, r->w, r->h, px, py, xmotion_resize);
}

void NeuroClientFreeMove(NeuroClientPtrPtr c, const void *free_setter_fn) {
  if (!c || !free_setter_fn)
    return;
  NeuroWorkspaceClientFocus(c, NeuroClientSelectorSelf, NULL);
  NeuroClientFree(c, free_setter_fn);
  NeuroRectangle *r = NeuroCoreClientGetRegion(c);
  int px = 0, py = 0;
  NeuroClientGetPointed(&px, &py);
  process_xmotion(r, CLI_GET(c).ws, r->x, r->y, r->w, r->h, px, py, xmotion_move);
}

void NeuroClientFreeResize(NeuroClientPtrPtr c, const void *free_setter_fn) {
  if (!c || !free_setter_fn)
    return;
  NeuroWorkspaceClientFocus(c, NeuroClientSelectorSelf, NULL);
  NeuroClientFree(c, free_setter_fn);
  NeuroRectangle *r = NeuroCoreClientGetRegion(c);
  int px = 0, py = 0;
  NeuroClientGetPointed(&px, &py);
  process_xmotion(r, CLI_GET(c).ws, r->x, r->y, r->w, r->h, px, py, xmotion_resize);
}

// Find
NeuroClientPtrPtr NeuroClientFindWindow(Window w) {
  return NeuroCoreFindClient(NeuroClientTesterWindow, (const void *)&w);
}

NeuroClientPtrPtr NeuroClientFindUrgent() {
  return NeuroCoreFindClient(NeuroClientTesterUrgent, NULL);
}

NeuroClientPtrPtr NeuroClientFindFixed() {
  return NeuroCoreFindClient(NeuroClientTesterFixed, NULL);
}

// NeuroClient getters
NeuroClientPtrPtr NeuroClientGetFocused() {
  return NeuroCoreStackGetCurrClient(NeuroCoreGetCurrStack());
}

NeuroClientPtrPtr NeuroClientGetPointed(int *x, int *y) {
  assert(x);
  assert(y);
  Window rootw, childw;
  int xc = 0, yc = 0;
  unsigned int state = 0;
  if (!XQueryPointer(NeuroSystemGetDisplay(), NeuroSystemGetRoot(), &rootw, &childw, x, y, &xc, &yc, &state))
    return NULL;
  return NeuroClientFindWindow(childw);
}

// NeuroClient Selectors
NeuroClientPtrPtr NeuroClientSelectorSelf(const NeuroClientPtrPtr c) {
  assert(c);
  return c;
}

NeuroClientPtrPtr NeuroClientSelectorNext(const NeuroClientPtrPtr c) {
  assert(c);
  NeuroClientPtrPtr n = NeuroCoreClientGetNext(c);
  if (!n)
    n = NeuroCoreStackGetHeadClient(CLI_GET(c).ws);
  return n;
}

NeuroClientPtrPtr NeuroClientSelectorPrev(const NeuroClientPtrPtr c) {
  assert(c);
  NeuroClientPtrPtr p = NeuroCoreClientGetPrev(c);
  if (!p)
    p = NeuroCoreStackGetLastClient(CLI_GET(c).ws);
  return p;
}

NeuroClientPtrPtr NeuroClientSelectorOld(const NeuroClientPtrPtr c) {
  assert(c);
  return NeuroCoreStackGetPrevClient(CLI_GET(c).ws);
}

NeuroClientPtrPtr NeuroClientSelectorHead(const NeuroClientPtrPtr c) {
  assert(c);
  return NeuroCoreStackGetHeadClient(CLI_GET(c).ws);
}

NeuroClientPtrPtr NeuroClientSelectorLast(const NeuroClientPtrPtr c) {
  assert(c);
  return NeuroCoreStackGetLastClient(CLI_GET(c).ws);
}

NeuroClientPtrPtr NeuroClientSelectorUp(const NeuroClientPtrPtr c) {
  assert(c);
  NeuroRectangle *r = NeuroCoreClientGetRegion(c);
  return query_pointer_client(CLI_GET(c).ws, r->x+1, r->y-1);
}

NeuroClientPtrPtr NeuroClientSelectorDown(const NeuroClientPtrPtr c) {
  assert(c);
  NeuroRectangle *r = NeuroCoreClientGetRegion(c);
  return query_pointer_client(CLI_GET(c).ws, r->x+1, r->y + r->h + 1);
}

NeuroClientPtrPtr NeuroClientSelectorLeft(const NeuroClientPtrPtr c) {
  assert(c);
  NeuroRectangle *r = NeuroCoreClientGetRegion(c);
  return query_pointer_client(CLI_GET(c).ws, r->x-1, r->y+1);
}

NeuroClientPtrPtr NeuroClientSelectorRight(const NeuroClientPtrPtr c) {
  assert(c);
  NeuroRectangle *r = NeuroCoreClientGetRegion(c);
  return query_pointer_client(CLI_GET(c).ws, r->x + r->w + 1, r->y+1);
}

// NeuroClient Testers
bool NeuroClientTesterWindow(const NeuroClientPtrPtr c, const void *w) {
  assert(c);
  assert(w);
  return CLI_GET(c).win == *((Window *)w);
}

bool NeuroClientTesterUrgent(const NeuroClientPtrPtr c, const void *data) {
  assert(c);
  (void)data;
  return CLI_GET(c).is_urgent;
}

bool NeuroClientTesterFixed(const NeuroClientPtrPtr c, const void *data) {
  assert(c);
  (void)data;
  return CLI_GET(c).fixed_pos != NEURO_FIXED_POSITION_NULL;
}

// NeuroColor Setters
NeuroColor NeuroClientColorSetterCurr(const NeuroClientPtrPtr c) {
  assert(c);
  if (NeuroCoreClientIsCurr(c))
    return NeuroSystemGetColor(NEURO_SYSTEM_COLOR_CURRENT);
  return NeuroSystemGetColor(NEURO_SYSTEM_COLOR_NORMAL);
}

NeuroColor NeuroClientColorSetterAll(const NeuroClientPtrPtr c) {
  assert(c);
  if (NeuroCoreClientIsCurr(c))
    return NeuroSystemGetColor(NEURO_SYSTEM_COLOR_CURRENT);
  else if (CLI_GET(c).is_urgent)
    return NeuroSystemGetColor(NEURO_SYSTEM_COLOR_URGENT);
  else if (CLI_GET(c).free_setter_fn != NeuroRuleFreeSetterNull)
    return NeuroSystemGetColor(NEURO_SYSTEM_COLOR_FREE);
  else if (NeuroCoreClientIsPrev(c))
    return NeuroSystemGetColor(NEURO_SYSTEM_COLOR_OLD);
  return NeuroSystemGetColor(NEURO_SYSTEM_COLOR_NORMAL);
}

NeuroColor NeuroClientColorSetterNone(const NeuroClientPtrPtr c) {
  (void)c;
  return NeuroSystemGetColor(NEURO_SYSTEM_COLOR_NORMAL);
}

// Border Width Setters
int NeuroClientBorderWidthSetterAlways(const NeuroClientPtrPtr c) {
  (void)c;
  return NeuroConfigGet()->border_width;
}

int NeuroClientBorderWidthSetterNever(const NeuroClientPtrPtr c) {
  (void)c;
  return 0;
}

int NeuroClientBorderWidthSetterSmart(const NeuroClientPtrPtr c) {
  assert(c);
  if (CLI_GET(c).is_fullscreen)
    return 0;
  if (CLI_GET(c).free_setter_fn != NeuroRuleFreeSetterNull)
    return NeuroConfigGet()->border_width;
  NeuroLayout *l = NeuroCoreStackGetCurrLayout(CLI_GET(c).ws);
  if (l->arranger_fn == NeuroLayoutArrangerFloat)
    return NeuroConfigGet()->border_width;
  if (NeuroWorkspaceClientFindFixed(CLI_GET(c).ws))
    return NeuroConfigGet()->border_width;
  NeuroRectangle *rc = NeuroCoreClientGetRegion(c);
  NeuroRectangle *rs = NeuroCoreStackGetRegion(CLI_GET(c).ws);
  if ((rc->w == rs->w && rc->h == rs->h) ||
      (rc->w == NeuroSystemGetScreenRegion()->w && rc->h == NeuroSystemGetScreenRegion()->h))
    return 0;
  return NeuroConfigGet()->border_width;
}

int NeuroClientBorderWidthSetterCurr(const NeuroClientPtrPtr c) {
  assert(c);
  if (NeuroCoreClientIsCurr(c))
    return NeuroConfigGet()->border_width;
  return 0;
}

// Border Gap Setters
int NeuroClientBorderGapSetterAlways(const NeuroClientPtrPtr c) {
  assert(c);
  if (CLI_GET(c).free_setter_fn != NeuroRuleFreeSetterNull)
    return 0;
  NeuroLayout *l = NeuroCoreStackGetCurrLayout(CLI_GET(c).ws);
  if (CLI_GET(c).is_fullscreen || CLI_GET(c).free_setter_fn != NeuroRuleFreeSetterNull ||
      l->arranger_fn == NeuroLayoutArrangerFloat)
    return 0;
  return NeuroConfigGet()->border_gap;
}

int NeuroClientBorderGapSetterNever(const NeuroClientPtrPtr c) {
  (void)c;
  return 0;
}

int NeuroClientBorderGapSetterSmart(const NeuroClientPtrPtr c) {
  assert(c);
  if (CLI_GET(c).free_setter_fn != NeuroRuleFreeSetterNull)
    return 0;
  NeuroLayout *l = NeuroCoreStackGetCurrLayout(CLI_GET(c).ws);
  if (CLI_GET(c).is_fullscreen || CLI_GET(c).free_setter_fn != NeuroRuleFreeSetterNull ||
      l->arranger_fn == NeuroLayoutArrangerFloat)
    return 0;
  NeuroRectangle *a = NeuroCoreClientGetRegion(c);
  NeuroRectangle *as = NeuroCoreStackGetRegion(CLI_GET(c).ws);
  if ((a->w == as->w && a->h == as->h) ||
      (a->w == NeuroSystemGetScreenRegion()->w && a->h == NeuroSystemGetScreenRegion()->h))
    return 0;
  return NeuroConfigGet()->border_gap;
}

int NeuroClientBorderGapSetterCurr(const NeuroClientPtrPtr c) {
  assert(c);
  if (CLI_GET(c).free_setter_fn != NeuroRuleFreeSetterNull)
    return 0;
  NeuroLayout *l = NeuroCoreStackGetCurrLayout(CLI_GET(c).ws);
  if (CLI_GET(c).is_fullscreen || CLI_GET(c).free_setter_fn != NeuroRuleFreeSetterNull ||
      l->arranger_fn == NeuroLayoutArrangerFloat)
    return 0;
  if (!NeuroCoreClientIsCurr(c))
    return 0;
  return NeuroConfigGet()->border_gap;
}

