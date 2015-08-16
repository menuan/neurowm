//----------------------------------------------------------------------------------------------------------------------
// Module      :  config
// Copyright   :  (c) Julian Bouzas 2014
// License     :  BSD3-style (see LICENSE)
// Maintainer  :  Julian Bouzas - nnoell3[at]gmail.com
// Stability   :  stable
//----------------------------------------------------------------------------------------------------------------------


//----------------------------------------------------------------------------------------------------------------------
// PREPROCESSOR
//----------------------------------------------------------------------------------------------------------------------

// Includes
#include "neurowm.h"


//----------------------------------------------------------------------------------------------------------------------
// PRIVATE VARIABLE DEFINITION
//----------------------------------------------------------------------------------------------------------------------

// Commands
static const char* termcmd[] = { "/usr/bin/xterm", NULL };
static const char* lchrcmd[] = { "/usr/bin/gmrun", NULL };

// Startup
const Action *defStartUpHook[] = { NULL };

// Endup
const Action *defEndUpHook[] = { NULL };

// Layouts
static const LayoutConf tile[] = { {
  "Tile", tallArrL, allBorderColorC, smartBorderWidthC, alwaysBorderGapC, {0.0f, 0.0f, 1.0f, 1.0f}, notModL, True,
  {ARG_INT(1), ARG_FLOAT(0.5f), ARG_FLOAT(0.03f), NO_ARG}
} };
static const LayoutConf mirr[] = { {
  "Mirr", tallArrL, allBorderColorC, smartBorderWidthC, alwaysBorderGapC, {0.0f, 0.0f, 1.0f, 1.0f}, mirrModL, True,
  {ARG_INT(1), ARG_FLOAT(0.5f), ARG_FLOAT(0.03f), NO_ARG}
} };
static const LayoutConf grid[] = { {
  "Grid", gridArrL, allBorderColorC, smartBorderWidthC, alwaysBorderGapC, {0.0f, 0.0f, 1.0f, 1.0f}, notModL, True,
  {NO_ARG, NO_ARG, NO_ARG, NO_ARG}
} };
static const LayoutConf full[] = { {
  "Full", fullArrL, allBorderColorC, smartBorderWidthC, alwaysBorderGapC, {0.0f, 0.0f, 1.0f, 1.0f}, notModL, True,
  {NO_ARG, NO_ARG, NO_ARG, NO_ARG}
} };
static const LayoutConf floa[] = { {
  "Float", floatArrL, allBorderColorC, alwaysBorderWidthC, alwaysBorderGapC, {0.0f, 0.0f, 1.0f, 1.0f}, notModL, False,
  {NO_ARG, NO_ARG, NO_ARG, NO_ARG}
} };

// Layouts
const LayoutConf *defLayoutSet[] = { tile, mirr, grid, NULL };

// TogLayoutSet
const LayoutConf *defTogLayoutSet[] = { full, floa, NULL };

// Workspaces
static const Workspace ws0[] = { { "ws1", {0, 0, 0, 0},  defLayoutSet, defTogLayoutSet } };
static const Workspace ws1[] = { { "ws2", {0, 0, 0, 0},  defLayoutSet, defTogLayoutSet } };
static const Workspace ws2[] = { { "ws3", {0, 0, 0, 0},  defLayoutSet, defTogLayoutSet } };
static const Workspace ws3[] = { { "ws4", {0, 0, 0, 0},  defLayoutSet, defTogLayoutSet } };
static const Workspace ws4[] = { { "ws5", {0, 0, 0, 0},  defLayoutSet, defTogLayoutSet } };
static const Workspace ws5[] = { { "ws6", {0, 0, 0, 0},  defLayoutSet, defTogLayoutSet } };
static const Workspace ws6[] = { { "ws7", {0, 0, 0, 0},  defLayoutSet, defTogLayoutSet } };
static const Workspace ws7[] = { { "ws8", {0, 0, 0, 0},  defLayoutSet, defTogLayoutSet } };
static const Workspace ws8[] = { { "ws9", {0, 0, 0, 0},  defLayoutSet, defTogLayoutSet } };
static const Workspace ws9[] = { { "ws0", {0, 0, 0, 0},  defLayoutSet, defTogLayoutSet } };

// WorkspaceSet
const Workspace *defWorkspaceSet[] = { ws0, ws1, ws2, ws3, ws4, ws5, ws6, ws7, ws8, ws9, NULL };

// RuleSet
const Rule *defRuleSet[] = { NULL };

// DzenPanelSet
const DzenPanel *defDzenPanelSet[] = { NULL };

// Key
static const Key key00[] = { { Mod1Mask|ShiftMask,   XK_Return, { spawnN, ARG_CMD(termcmd) }                    } };
static const Key key01[] = { { Mod1Mask,             XK_c,      { killCurrClientN, ARG_SCF(selfC) }             } };
static const Key key02[] = { { Mod1Mask,             XK_j,      { moveFocusCurrClientN, ARG_SCF(nextC) }        } };
static const Key key03[] = { { Mod1Mask,             XK_k,      { moveFocusCurrClientN, ARG_SCF(prevC) }        } };
static const Key key04[] = { { Mod1Mask,             XK_Tab,    { moveFocusCurrClientN, ARG_SCF(oldC) }         } };
static const Key key05[] = { { Mod1Mask,             XK_m,      { moveFocusCurrClientN, ARG_SCF(headC) }        } };
static const Key key06[] = { { Mod1Mask|ShiftMask,   XK_j,      { swapCurrClientN, ARG_SCF(nextC) }             } };
static const Key key07[] = { { Mod1Mask|ShiftMask,   XK_k,      { swapCurrClientN, ARG_SCF(prevC) }             } };
static const Key key08[] = { { Mod1Mask|ShiftMask,   XK_p,      { swapCurrClientN, ARG_SCF(oldC) }              } };
static const Key key09[] = { { Mod1Mask|ShiftMask,   XK_m,      { swapCurrClientN, ARG_SCF(headC) }             } };
static const Key key10[] = { { Mod1Mask,             XK_Up,     { moveFocusCurrClientN, ARG_SCF(upC) }          } };
static const Key key11[] = { { Mod1Mask,             XK_Down,   { moveFocusCurrClientN, ARG_SCF(downC) }        } };
static const Key key12[] = { { Mod1Mask,             XK_Left,   { moveFocusCurrClientN, ARG_SCF(leftC) }        } };
static const Key key13[] = { { Mod1Mask,             XK_Right,  { moveFocusCurrClientN, ARG_SCF(rightC) }       } };
static const Key key14[] = { { Mod1Mask|ShiftMask,   XK_Up,     { swapCurrClientN, ARG_SCF(upC) }               } };
static const Key key15[] = { { Mod1Mask|ShiftMask,   XK_Down,   { swapCurrClientN, ARG_SCF(downC) }             } };
static const Key key16[] = { { Mod1Mask|ShiftMask,   XK_Left,   { swapCurrClientN, ARG_SCF(leftC) }             } };
static const Key key17[] = { { Mod1Mask|ShiftMask,   XK_Right,  { swapCurrClientN, ARG_SCF(rightC) }            } };
static const Key key18[] = { { Mod1Mask,             XK_space,  { changeLayoutN, ARG_INT(1) }                   } };
static const Key key19[] = { { Mod1Mask,             XK_t,      { toggleFreeCurrClientN, ARG_FLF(defFreeR) }    } };
static const Key key20[] = { { Mod1Mask,             XK_z,      { toggleLayoutModN, ARG_UINT(mirrModL)  }       } };
static const Key key21[] = { { Mod1Mask,             XK_x,      { toggleLayoutModN, ARG_UINT(reflXModL) }       } };
static const Key key22[] = { { Mod1Mask,             XK_y,      { toggleLayoutModN, ARG_UINT(reflYModL) }       } };
static const Key key23[] = { { Mod1Mask,             XK_f,      { toggleLayoutN, ARG_INT(0) }                   } };
static const Key key24[] = { { Mod1Mask,             XK_o,      { toggleLayoutN, ARG_INT(1) }                   } };
static const Key key25[] = { { Mod1Mask|ShiftMask,   XK_f,      { toggleFullScreenCurrClientN, ARG_SCF(selfC) } } };
static const Key key26[] = { { Mod1Mask,             XK_comma,  { increaseMasterLayoutN, ARG_INT(1) }           } };
static const Key key27[] = { { Mod1Mask,             XK_period, { increaseMasterLayoutN, ARG_INT(-1) }          } };
static const Key key28[] = { { Mod1Mask,             XK_l,      { resizeMasterLayoutN, ARG_FLOAT(1.0f) }        } };
static const Key key29[] = { { Mod1Mask,             XK_h,      { resizeMasterLayoutN, ARG_FLOAT(-1.0f) }       } };
static const Key key30[] = { { Mod1Mask,             XK_1,      { changeToWorkspaceN, ARG_INT(0) }              } };
static const Key key31[] = { { Mod1Mask,             XK_2,      { changeToWorkspaceN, ARG_INT(1) }              } };
static const Key key32[] = { { Mod1Mask,             XK_3,      { changeToWorkspaceN, ARG_INT(2) }              } };
static const Key key33[] = { { Mod1Mask,             XK_4,      { changeToWorkspaceN, ARG_INT(3) }              } };
static const Key key34[] = { { Mod1Mask,             XK_5,      { changeToWorkspaceN, ARG_INT(4) }              } };
static const Key key35[] = { { Mod1Mask,             XK_6,      { changeToWorkspaceN, ARG_INT(5) }              } };
static const Key key36[] = { { Mod1Mask,             XK_7,      { changeToWorkspaceN, ARG_INT(6) }              } };
static const Key key37[] = { { Mod1Mask,             XK_8,      { changeToWorkspaceN, ARG_INT(7) }              } };
static const Key key38[] = { { Mod1Mask,             XK_9,      { changeToWorkspaceN, ARG_INT(8) }              } };
static const Key key39[] = { { Mod1Mask,             XK_0,      { changeToWorkspaceN, ARG_INT(9) }              } };
static const Key key40[] = { { ControlMask|Mod1Mask, XK_Left,   { changeToPrevWorkspaceN, NO_ARG }              } };
static const Key key41[] = { { ControlMask|Mod1Mask, XK_Right,  { changeToNextWorkspaceN, NO_ARG }              } };
static const Key key42[] = { { Mod1Mask|ShiftMask,   XK_Tab,    { changeToLastWorkspaceN, NO_ARG }              } };
static const Key key43[] = { { Mod1Mask|ShiftMask,   XK_1,      { moveClientToWorkspaceN, ARG_INT(0) }          } };
static const Key key44[] = { { Mod1Mask|ShiftMask,   XK_2,      { moveClientToWorkspaceN, ARG_INT(1) }          } };
static const Key key45[] = { { Mod1Mask|ShiftMask,   XK_3,      { moveClientToWorkspaceN, ARG_INT(2) }          } };
static const Key key46[] = { { Mod1Mask|ShiftMask,   XK_4,      { moveClientToWorkspaceN, ARG_INT(3) }          } };
static const Key key47[] = { { Mod1Mask|ShiftMask,   XK_5,      { moveClientToWorkspaceN, ARG_INT(4) }          } };
static const Key key48[] = { { Mod1Mask|ShiftMask,   XK_6,      { moveClientToWorkspaceN, ARG_INT(5) }          } };
static const Key key49[] = { { Mod1Mask|ShiftMask,   XK_7,      { moveClientToWorkspaceN, ARG_INT(6) }          } };
static const Key key50[] = { { Mod1Mask|ShiftMask,   XK_8,      { moveClientToWorkspaceN, ARG_INT(7) }          } };
static const Key key51[] = { { Mod1Mask|ShiftMask,   XK_9,      { moveClientToWorkspaceN, ARG_INT(8) }          } };
static const Key key52[] = { { Mod1Mask|ShiftMask,   XK_0,      { moveClientToWorkspaceN, ARG_INT(9) }          } };
static const Key key53[] = { { Mod1Mask|ShiftMask,   XK_q,      { quitN, NO_ARG }                               } };
static const Key key54[] = { { Mod1Mask,             XK_q,      { reloadN, NO_ARG }                             } };
static const Key key55[] = { { Mod1Mask,             XK_F2,     { spawnN, ARG_CMD(lchrcmd) }                    } };
static const Key key56[] = { { Mod1Mask|ShiftMask,   XK_space,  { resetLayoutN, NO_ARG }                        } };
static const Key key57[] = { { Mod1Mask,             XK_n,      { minimizeCurrClientN, ARG_SCF(selfC) }         } };
static const Key key58[] = { { Mod1Mask|ShiftMask,   XK_n,      { restoreLastMinimizedN, NO_ARG }               } };

// Keys
const Key *defKeys[] = { key00, key01, key02, key03, key04, key05, key06, key07, key08, key09,
                         key10, key11, key12, key13, key14, key15, key16, key17, key18, key19,
                         key20, key21, key22, key23, key24, key25, key26, key27, key28, key29,
                         key30, key31, key32, key33, key34, key35, key36, key37, key38, key39,
                         key40, key41, key42, key43, key44, key45, key46, key47, key48, key49,
                         key50, key51, key52, key53, key54, key55, key56, key57, key58, NULL };

// Button
static const Button button00[] = { { noModMask,          Button1, { moveFocusPtrClientN, ARG_SCF(selfC) },      True  } };
static const Button button01[] = { { Mod1Mask,           Button1, { freeMovePtrClientN, NO_ARG },               False } };
static const Button button02[] = { { Mod1Mask,           Button2, { toggleFreePtrClientN, ARG_FLF(defFreeR) },  False } };
static const Button button03[] = { { Mod1Mask,           Button3, { freeResizePtrClientN, NO_ARG },             False } };
static const Button button04[] = { { Mod1Mask|ShiftMask, Button1, { movePtrClientN, NO_ARG },                   False } };
static const Button button05[] = { { Mod1Mask|ShiftMask, Button2, { toggleFullScreenPtrClientN, NO_ARG },       False } };
static const Button button06[] = { { Mod1Mask|ShiftMask, Button3, { resizePtrClientN, NO_ARG },                 False } };

// Buttons
const Button *defButtons[] = { button00, button01, button02, button03, button04, button05, button05, NULL };


//----------------------------------------------------------------------------------------------------------------------
// PUBLIC VARIABLE DEFINITION
//----------------------------------------------------------------------------------------------------------------------

const WMConfig defWMConfig = {
  defNormBorderColor,
  defCurrBorderColor,
  defPrevBorderColor,
  defFreeBorderColor,
  defUrgtBorderColor,
  defBorderWidth,
  defBorderGap,
  defWorkspaceSet,
  defRuleSet,
  defDzenPanelSet,
  defKeys,
  defButtons,
  defStartUpHook,
  defEndUpHook
};

