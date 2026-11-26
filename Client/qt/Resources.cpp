#include "Resources.h"

#include <QBitmap>
#include <QCursor>
#include <QFile>
#include <QImage>
#include <QtEndian>
#include <map>

namespace Resources {

static const MenuEntry menu103[] = {
	{MENU_POPUP, "Selection"},
	{MENU_ITEM, "Cu&t\tCtrl+X", 57635},
	{MENU_ITEM, "&Copy\tCtrl+C", 57634},
	{MENU_ITEM, "&Paste Into\tCtrl+V", 33015},
	{MENU_SEPARATOR},
	{MENU_ITEM, "&Group", 32825},
	{MENU_ITEM, "&Ungroup", 32826},
	{MENU_SEPARATOR},
	{MENU_ITEM, "Insert Into from File...", 32801},
	{MENU_SEPARATOR},
	{MENU_ITEM, "&Save to File...", 32805},
	{MENU_ITEM, "Save to Roblox...", 32951},
	{MENU_END},
};

static const MenuEntry menu128[] = {
	{MENU_POPUP, "&File"},
	{MENU_ITEM, "&New\tCtrl+N", 57600},
	{MENU_ITEM, "&Open...\tCtrl+O", 57601},
	{MENU_ITEM, "&Close", 57602},
	{MENU_SEPARATOR},
	{MENU_ITEM, "&Save\tCtrl+S", 57603},
	{MENU_ITEM, "Save &As...", 57604},
	{MENU_ITEM, "Publish to ROBLOX...", 32950},
	{MENU_SEPARATOR},
	{MENU_ITEM, "&Save Selection as Model...", 32805},
	{MENU_ITEM, "Publish Selection to ROBLOX...", 32951},
	{MENU_SEPARATOR},
	{MENU_ITEM, "&Print...\tCtrl+P", 57607},
	{MENU_ITEM, "Print Pre&view", 57609},
	{MENU_ITEM, "P&rint Setup...", 57606},
	{MENU_SEPARATOR},
	{MENU_ITEM, "Recent File", 57616, MENU_GRAYED},
	{MENU_SEPARATOR},
	{MENU_ITEM, "E&xit", 57665},
	{MENU_END},
	{MENU_POPUP, "&Edit"},
	{MENU_ITEM, "&Undo\tCtrl+Z", 57643},
	{MENU_ITEM, "&Redo\tCtrl+Y", 57644},
	{MENU_SEPARATOR},
	{MENU_ITEM, "Cu&t\tCtrl+X", 57635},
	{MENU_ITEM, "&Copy\tCtrl+C", 57634},
	{MENU_ITEM, "&Paste\tCtrl+V", 57637},
	{MENU_SEPARATOR},
	{MENU_ITEM, "Clear\tDel", 57632},
	{MENU_ITEM, "Select &All\tCtrl+A", 57642},
	{MENU_ITEM, "&Duplicate\tCtrl+D", 32773},
	{MENU_SEPARATOR},
	{MENU_POPUP, "Find and Replace"},
	{MENU_ITEM, "&Find...\tCtrl+F", 57636},
	{MENU_ITEM, "Find &Next\tF3", 57640},
	{MENU_ITEM, "&Replace...\tCtrl+H", 57641},
	{MENU_END},
	{MENU_POPUP, "Script"},
	{MENU_ITEM, "Display &Line Numbers", 32994},
	{MENU_ITEM, "Display &Selection/Bookmark Margin", 32995},
	{MENU_ITEM, "Display &folding Margin", 32996},
	{MENU_SEPARATOR},
	{MENU_ITEM, "&Add a Marker", 32997},
	{MENU_ITEM, "&Delete Marker", 32998},
	{MENU_ITEM, "Find &Next Marker", 32999},
	{MENU_ITEM, "Find &Previous Marker", 33000},
	{MENU_END},
	{MENU_SEPARATOR},
	{MENU_ITEM, "Lock", 32784},
	{MENU_ITEM, "Unlock All", 32785},
	{MENU_END},
	{MENU_POPUP, "&View"},
	{MENU_ITEM, "&Explorer", 32792},
	{MENU_ITEM, "&Properties", 32783},
	{MENU_ITEM, "&Output", 32927},
	{MENU_ITEM, "&Start Page", 32959},
	{MENU_ITEM, "&Report", 33011},
	{MENU_ITEM, "Toolbox", 33022},
	{MENU_SEPARATOR},
	{MENU_ITEM, "Game Layout", 33013},
	{MENU_ITEM, "Full Screen", 33012},
	{MENU_POPUP, "&Toolbars"},
	{MENU_ITEM, "&Toolbar", 59392},
	{MENU_SEPARATOR},
	{MENU_ITEM, "Customize...", 35001},
	{MENU_END},
	{MENU_END},
	{MENU_POPUP, "&Insert"},
	{MENU_ITEM, "Model...", 32800},
	{MENU_ITEM, "LDraw file...", 33016},
	{MENU_SEPARATOR},
	{MENU_ITEM, "Decal...", 32866},
	{MENU_ITEM, "Texture...", 33006},
	{MENU_ITEM, "Object...", 32895},
	{MENU_ITEM, "Tool", 32780},
	{MENU_SEPARATOR},
	{MENU_ITEM, "Clear Tools in Starterpack", 32781},
	{MENU_ITEM, "Clear Tools in My Backpack", 32782},
	{MENU_END},
	{MENU_POPUP, "For&mat"},
	{MENU_ITEM, "Rotate\tCtrl+R", 32835},
	{MENU_ITEM, "Tilt", 32836},
	{MENU_POPUP, "Camera Positioning"},
	{MENU_ITEM, "Fixed", 32815},
	{MENU_ITEM, "Watching", 32930},
	{MENU_ITEM, "Attached", 32816},
	{MENU_ITEM, "Tracking", 32931},
	{MENU_ITEM, "Following", 32818},
	{MENU_END},
	{MENU_SEPARATOR},
	{MENU_ITEM, "&Group\tCtrl+G", 32825},
	{MENU_ITEM, "&Ungroup\tCtrl+U", 32826},
	{MENU_SEPARATOR},
	{MENU_ITEM, "Set Model Front", 32938},
	{MENU_ITEM, "Set Model Primary Part", 32939},
	{MENU_SEPARATOR},
	{MENU_ITEM, "Replace Glue with Welds", 32865},
	{MENU_ITEM, "Replace Weld with Snaps", 32933},
	{MENU_END},
	{MENU_POPUP, "&Tools"},
	{MENU_ITEM, "E&xecute Script...", 33009},
	{MENU_ITEM, "&GC", 32986},
	{MENU_SEPARATOR},
	{MENU_POPUP, "Test"},
	{MENU_ITEM, "Solo", 33028},
	{MENU_ITEM, "1 Player", 33029},
	{MENU_ITEM, "2 Players", 33030},
	{MENU_END},
	{MENU_ITEM, "Join Selected Bricks", 32984},
	{MENU_ITEM, "Force Assertion", 32980},
	{MENU_ITEM, "Engine Stats", 32981},
	{MENU_ITEM, "Insert Model Raw...", 32982},
	{MENU_ITEM, "Image Server Model View", 32983},
	{MENU_ITEM, "Report Abuse...", 33024},
	{MENU_ITEM, "Report Crashes...", 33025},
	{MENU_SEPARATOR},
	{MENU_ITEM, "&Customize...", 35001},
	{MENU_ITEM, "&Settings...", 32989},
	{MENU_END},
	{MENU_POPUP, "&Window"},
	{MENU_ITEM, "Window List", 35000},
	{MENU_END},
	{MENU_POPUP, "&Help"},
	{MENU_ITEM, "&About Roblox...", 57664},
	{MENU_ITEM, "Online &Help...\tF1", 57670},
	{MENU_ITEM, "&Object Browser", 32990},
	{MENU_END},
};

static const MenuEntry menu129[] = {
	{MENU_POPUP, "File"},
	{MENU_ITEM, "&Close", 57602},
	{MENU_END},
};

static const MenuEntry menu139[] = {
	{MENU_POPUP, "Undo"},
	{MENU_ITEM, "Undo1", 32917},
	{MENU_ITEM, "Undo2", 32918},
	{MENU_ITEM, "Undo3", 32919},
	{MENU_ITEM, "Undo4", 32920},
	{MENU_ITEM, "Undo5", 32921},
	{MENU_END},
};

static const MenuEntry menu140[] = {
	{MENU_POPUP, "Redo"},
	{MENU_ITEM, "Redo1", 32912},
	{MENU_ITEM, "Redo2", 32913},
	{MENU_ITEM, "Redo3", 32914},
	{MENU_ITEM, "Redo4", 32915},
	{MENU_ITEM, "Redo5", 32916},
	{MENU_END},
};

static const MenuEntry menu185[] = {
	{MENU_POPUP, "File"},
	{MENU_ITEM, "&Close", 57602},
	{MENU_END},
};

static const MenuEntry menu186[] = {
	{MENU_POPUP, "&File"},
	{MENU_ITEM, "E&xit", 57665},
	{MENU_END},
	{MENU_POPUP, "&Tools"},
	{MENU_ITEM, "&Settings...", 32989},
	{MENU_END},
	{MENU_POPUP, "&Help"},
	{MENU_ITEM, "&About Roblox...", 57664},
	{MENU_END},
};

static const MenuEntry menu9121[] = {
	{MENU_POPUP, "Customize"},
	{MENU_ITEM, "&Reset", 9123},
	{MENU_ITEM, "&Delete", 9124},
	{MENU_SEPARATOR},
	{MENU_ITEM, "&Name:", 9125},
	{MENU_SEPARATOR},
	{MENU_ITEM, "&Copy Button Image", 9126},
	{MENU_ITEM, "&Paste Button Image", 9127},
	{MENU_ITEM, "Re&set Button Image", 9128},
	{MENU_ITEM, "&Edit Button Image...", 9129},
	{MENU_ITEM, "Change &Button Image", 9130},
	{MENU_SEPARATOR},
	{MENU_ITEM, "Defa&ult Style", 9131},
	{MENU_ITEM, "&Text Only", 9133},
	{MENU_ITEM, "Image &and Text", 9132},
	{MENU_SEPARATOR},
	{MENU_ITEM, "Begin a &Group", 9134},
	{MENU_END},
};

static const DialogControl dialog101Controls[] = {
	{"Fi&nd what:", -1, "Static", 0x50020000, 9, 8, 42, 8},
	{"", 1152, "Edit", 0x50830080, 47, 8, 128, 12},
	{"Match &whole word only", 1040, "Button", 0x50030003, 9, 26, 100, 12},
	{"Match &case", 1041, "Button", 0x50010003, 9, 42, 64, 12},
	{"Regular &expression", 1042, "Button", 0x50010003, 9, 58, 83, 12},
	{"Direction", 1072, "Button", 0x50020007, 107, 26, 68, 28},
	{"&Up", 1056, "Button", 0x50020009, 111, 38, 25, 12},
	{"&Down", 1057, "Button", 0x50000009, 138, 38, 35, 12},
	{"&Find Next", 1, "Button", 0x50030001, 187, 8, 50, 14},
	{"Close", 2, "Button", 0x50010000, 187, 30, 50, 14},
	{"&Help", 1038, "Button", 0x50010000, 187, 55, 50, 14},
};

static const DialogControl dialog102Controls[] = {
	{"Fi&nd what:", -1, "Static", 0x50020000, 10, 9, 48, 8},
	{"", 1152, "Edit", 0x50830080, 54, 9, 114, 12},
	{"Re&place with:", -1, "Static", 0x50020000, 10, 30, 48, 8},
	{"", 1153, "Edit", 0x50830080, 54, 28, 114, 12},
	{"Match &whole word only", 1040, "Button", 0x50030003, 10, 53, 104, 12},
	{"Match &case", 1041, "Button", 0x50010003, 10, 70, 59, 12},
	{"Regular &expression", 1042, "Button", 0x50010003, 10, 86, 83, 12},
	{"&Find Next", 1, "Button", 0x50030001, 190, 9, 50, 14},
	{"&Replace", 1024, "Button", 0x50010000, 190, 28, 50, 14},
	{"Replace &All", 1025, "Button", 0x50010000, 190, 46, 50, 14},
	{"Close", 2, "Button", 0x50010000, 190, 64, 50, 14},
	{"&Help", 1038, "Button", 0x50010000, 190, 84, 50, 14},
};

static const DialogControl dialog107Controls[] = {
	{"Class:", -1, "Static", 0x50020000, 7, 7, 40, 14, 0x00001000},
	{"", 1000, "Edit", 0x50810080, 53, 7, 175, 14},
	{"OK", 1, "Button", 0x50010001, 249, 7, 50, 16},
	{"Cancel", 2, "Button", 0x50010000, 249, 26, 50, 16},
};

static const DialogControl dialog122Controls[] = {
	{"Close", 1, "Button", 0x50010001, 313, 214, 50, 14},
	{"", 1001, "Static", 0x40000007, 7, 7, 356, 196},
	{"Reset All Settings", 1002, "Button", 0x50010000, 7, 214, 81, 14},
};

static const DialogControl dialog9115Controls[] = {
	{"Picture", -1, "Static", 0x50020000, 7, 7, 86, 8},
	{"Colors", -1, "Button", 0x50000007, 102, 106, 80, 71},
	{"Move", -1, "Button", 0x50000007, 101, 72, 81, 31},
	{"Preview", -1, "Button", 0x50000007, 7, 106, 86, 71},
	{"Tools", -1, "Button", 0x50000007, 101, 9, 81, 60},
	{"OK", 1, "Button", 0x50010001, 193, 7, 50, 14},
	{"Cancel", 2, "Button", 0x50010000, 193, 25, 50, 14},
	{"", 100, "Static", 0x50000107, 7, 17, 86, 80},
	{"", 101, "Static", 0x50020201, 14, 117, 73, 54, 0x00020000},
	{"Transparent", -1, "Static", 0x50020000, 135, 156, 42, 8},
	{"", 105, "Static", 0x50020100, 116, 153, 15, 15},
	{"Tools", 103, "Static", 0x40820201, 112, 21, 60, 43},
	{"Move", 104, "Static", 0x40820201, 111, 83, 62, 12},
	{"", 106, "Static", 0x5002010c, 109, 144, 15, 15},
	{"Colors", 102, "Static", 0x40820201, 107, 121, 68, 12},
};

static const DialogControl dialog9117Controls[] = {
	{"Toolb&ars:", -1, "Static", 0x50020000, 4, 5, 148, 8},
	{"", 100, "ListBox", 0x50a10151, 4, 15, 150, 131},
	{"&New...", 101, "Button", 0x50010000, 158, 15, 70, 14},
	{"&Rename...", 102, "Button", 0x50010000, 158, 33, 70, 14},
	{"&Delete", 103, "Button", 0x50010000, 158, 51, 70, 14},
	{"R&eset...", 104, "Button", 0x50010000, 158, 69, 70, 14},
};

static const DialogControl dialog9118Controls[] = {
	{"Personalized Menus and Toolbars", 108, "Static", 0x50020000, 4, 5, 222, 8},
	{"Always show full me&nus", 100, "Button", 0x50010003, 10, 17, 216, 10},
	{"Show full menus after a short &delay", 101, "Button", 0x50010003, 22, 29, 204, 10},
	{"&Reset menu and toolbar usage data", 102, "Button", 0x50010000, 10, 42, 125, 14},
	{"Other", 107, "Static", 0x50020000, 4, 70, 222, 8},
	{"&Large icons", 103, "Button", 0x50010003, 10, 82, 216, 10},
	{"Show Screen&Tips on toolbars", 104, "Button", 0x50010003, 10, 93, 216, 10},
	{"Show s&hortcut keys in ScreenTips", 105, "Button", 0x50010003, 22, 104, 204, 10},
	{"&Menu animations:", -1, "Static", 0x50020000, 10, 117, 56, 8},
	{"", 106, "ComboBox", 0x50210003, 70, 115, 79, 65},
};

static const DialogControl dialog9119Controls[] = {
	{"To add a command to a toolbar: select a category and drag the command out of this dialog box to a toolbar.",
	 -1,
	 "Static",
	 0x50020000,
	 4,
	 5,
	 220,
	 19},
	{"Cate&gories:", -1, "Static", 0x50020000, 4, 25, 80, 8},
	{"", 100, "ListBox", 0x50a10101, 4, 35, 81, 111},
	{"Comman&ds:", -1, "Static", 0x50020000, 90, 25, 134, 8},
	{"", 101, "ListBox", 0x50a10111, 90, 35, 135, 111},
};

static const DialogControl dialog9120Controls[] = {
	{"&Toolbar name:", -1, "Static", 0x50020000, 4, 4, 178, 8},
	{"", 100, "Edit", 0x50810080, 4, 15, 178, 12},
	{"OK", 1, "Button", 0x50010001, 78, 34, 50, 14},
	{"Cancel", 2, "Button", 0x50010000, 132, 34, 50, 14},
};

static const DialogControl dialog30722Controls[] = {
	{"Printing", -1, "Static", 0x50020001, 6, 7, 168, 8},
	{"Document :", -1, "Static", 0x50020002, 6, 19, 65, 8},
	{"", 201, "Static", 0x50020000, 79, 19, 95, 8},
	{"Page :", -1, "Static", 0x50020002, 6, 31, 65, 8},
	{"", 204, "Static", 0x50020000, 79, 31, 95, 8},
	{"Printer :", -1, "Static", 0x50020002, 6, 43, 65, 8},
	{"", 202, "Static", 0x50020000, 79, 43, 95, 8},
	{"Port :", -1, "Static", 0x50020002, 6, 55, 65, 8},
	{"", 203, "Static", 0x50020000, 79, 55, 95, 8},
	{"Cancel", 2, "Button", 0x50010000, 70, 67, 40, 14},
};

static const DialogControl dialog30723Controls[] = {
	{"&Print...", 58116, "Button", 0x50010000, 2, 2, 44, 12},
	{"&Next Page", 58114, "Button", 0x50010000, 48, 2, 44, 12},
	{"Pre&v Page", 58115, "Button", 0x50010000, 94, 2, 44, 12},
	{"", 58113, "Button", 0x50010000, 140, 2, 44, 12},
	{"Zoom &In", 58117, "Button", 0x50010000, 186, 2, 44, 12},
	{"Zoom &Out", 58118, "Button", 0x50010000, 232, 2, 44, 12},
	{"&Close", 58112, "Button", 0x50010000, 278, 2, 44, 12},
};

static const DialogTemplate dialogs[] = {
	{101, 30, 73, 246, 79, 0x80c820cc, 0x00000080, "Find", 8, "MS Shell Dlg", dialog101Controls},
	{102, 36, 44, 250, 108, 0x80c820cc, 0x00000080, "Replace", 8, "MS Shell Dlg", dialog102Controls},
	{107, 0, 0, 306, 49, 0x80c800c0, 0, "Dialog", 8, "MS Sans Serif", dialog107Controls},
	{113, 0, 0, 320, 200, 0x80c800c0, 0, "Dialog", 8, "MS Sans Serif", {}},
	{117, 0, 0, 295, 195, 0x80c800c0, 0, "", 8, "MS Sans Serif", {}},
	{120, 0, 0, 279, 297, 0x80c800c0, 0x00000300, "Dialog", 8, "MS Sans Serif", {}},
	{122, 0, 0, 370, 235, 0x80c800c0, 0, "Settings", 8, "MS Sans Serif", dialog122Controls},
	{123, 0, 0, 232, 143, 0x80c800c0, 0, "About ROBLOX", 8, "MS Sans Serif", {}},
	{124, 0, 0, 353, 121, 0x80c800c0, 0, "ROBLOX", 8, "MS Sans Serif", {}},
	{9115, 0, 0, 253, 185, 0x90c800c8, 0x00000000, "Button Editor", 8, "MS Shell Dlg", dialog9115Controls},
	{9117, 0, 0, 230, 150, 0x40c00048, 0x00000000, "Tool&bars", 8, "MS Shell Dlg", dialog9117Controls},
	{9118, 0, 0, 230, 150, 0x40c00048, 0x00000000, "&Options", 8, "MS Shell Dlg", dialog9118Controls},
	{9119, 0, 0, 230, 150, 0x40c00048, 0x00000000, "&Commands", 8, "MS Shell Dlg", dialog9119Controls},
	{9120, 0, 0, 186, 52, 0x80c800c8, 0x00000000, "New Toolbar", 8, "MS Shell Dlg", dialog9120Controls},
	{30722, 6, 18, 181, 87, 0x80c000c4, 0x00000000, "", 8, "MS Shell Dlg", dialog30722Controls},
	{30723, 0, 0, 330, 16, 0x40000044, 0x00000000, "", 8, "MS Shell Dlg", dialog30723Controls},
};

static const unsigned toolBar128Buttons[] = {
	57600,
	57601,
	57603,
	0,
	57635,
	57634,
	57637,
	0,
	57643,
	57644,
	0,
	32825,
	32826,
	0,
	32835,
	32836,
	32855,
	32856,
};

static const unsigned toolBar135Buttons[] = {
	32867,
	32871,
	32869,
	32872,
	0,
	32883,
	32884,
	0,
	32832,
	32833,
	0,
	32888,
	32887,
	32889,
	32935,
	32936,
	32892,
	32891,
};

static const unsigned toolBar137Buttons[] = {32965, 32966, 32967, 32968};

static const unsigned toolBar145Buttons[] = {32807, 32808, 32809, 32810, 32811, 32812, 32934, 32813};

static const unsigned toolBar147Buttons[] = {32954, 32953, 32955, 32956, 32957, 32969, 32970};

static const unsigned toolBar150Buttons[] = {32963, 32987, 33014, 32986};

static const ToolBar toolBars[] = {
	{128, 15, 16, toolBar128Buttons},
	{135, 16, 16, toolBar135Buttons},
	{137, 16, 16, toolBar137Buttons},
	{145, 16, 16, toolBar145Buttons},
	{147, 16, 16, toolBar147Buttons},
	{150, 16, 15, toolBar150Buttons},
};

static const Accelerator accelerators128[] = {
	{46, 57632, 0},
	{67, 57634, ACCEL_CONTROL},
	{45, 57634, ACCEL_CONTROL},
	{46, 57635, ACCEL_SHIFT},
	{88, 57635, ACCEL_CONTROL},
	{68, 32773, ACCEL_CONTROL},
	{86, 57637, ACCEL_CONTROL},
	{45, 57637, ACCEL_SHIFT},
	{89, 57644, ACCEL_CONTROL},
	{90, 57644, ACCEL_SHIFT | ACCEL_CONTROL},
	{65, 57642, ACCEL_CONTROL},
	{8, 57643, ACCEL_ALT},
	{90, 57643, ACCEL_CONTROL},
	{78, 57600, ACCEL_CONTROL},
	{79, 57601, ACCEL_CONTROL},
	{80, 57607, ACCEL_CONTROL},
	{83, 57603, ACCEL_CONTROL},
	{71, 32825, ACCEL_CONTROL},
	{82, 32835, ACCEL_CONTROL},
	{85, 32826, ACCEL_CONTROL},
	{112, 57670, 0},
	{117, 57680, 0},
	{117, 57681, ACCEL_SHIFT},
	{70, 57636, ACCEL_CONTROL},
	{114, 57640, 0},
	{72, 57641, ACCEL_CONTROL},
};

static const Accelerator accelerators207[] = {
	{67, 57634, ACCEL_CONTROL},
	{45, 57634, ACCEL_CONTROL},
	{46, 57635, ACCEL_SHIFT},
	{88, 57635, ACCEL_CONTROL},
	{86, 57637, ACCEL_CONTROL},
	{45, 57637, ACCEL_SHIFT},
	{8, 57643, ACCEL_ALT},
	{90, 57643, ACCEL_CONTROL},
	{80, 57607, ACCEL_CONTROL},
};

static const std::map<unsigned, const char*> strings = {
	{128, "Roblox"},
	{129, "ROBLOX\nPlace\nPlace\nPlaces (*.rbxl)\n.rbxl\nRoblox.Level\nRoblox.Level"},
	{130, "&Undo %s\tCtrl+Z"},
	{131, "&Redo %s\tCtrl+Y"},
	{132,
	 "Images (*.jpg;*.jpeg;*.tga;*.bmp;*.ico;*.png)|*.jpg; *.jpeg; *.tga; *.bmp; *.ico; *.png|All Files (*.*)|*.*||"},
	{133, "Failed to insert object of type \"%s\". Did you spell it correctly?"},
	{134, "Sounds (*.wav;*.mp3;*.mid;*.ogg;*.wma)|*.wav; *.mp3; *.mid; *.ogg; *.wma|All Files (*.*)|*.*||"},
	{135, "Pointer"},
	{136,
	 "Changing settings may make ROBLOX unstable.\n\nWe recommend that you don't change any ROBLOX settings unless "
	 "instructed to do so by a ROBLOX engineer."},
	{137, "Failed to create Service object of type \"%s\""},
	{138, "Object Browser\nObject Browser\n\n\n\n\n"},
	{139, "General"},
	{140, "Report"},
	{141, "LDraw (*.ldr;*.dat;*.mpd;*.txt)|*.ldr; *.dat; *.mpd; *.txt|All Files (*.*)|*.*||"},
	{142, "Some setting changes will not take effect until you restart ROBLOX"},
	{143, "Resetting settings will not take effect until you restart ROBLOX"},
	{180, "%s \"%s\""},
	{181, "%s \"%s\" - %d items"},
	{182, "%s - %d items"},
	{183, "%d items"},
	{184, "Models (*.rbxm)|*.rbxm|All Files (*.*)|*.*||"},
	{185, "Browser\n\n\n\n\n\n"},
	{186, "Frame"},
	{187, "00000"},
	{188, "Tex 000MB"},
	{189, "VAR 000MB"},
	{190, "000FPS"},
	{191, "Toolbox"},
	{192, "Output"},
	{193, "Properties"},
	{194, "Explorer"},
	{196, "ROBLOX needs to quit for the installation to proceed"},
	{197, "%1 - Printed on %2"},
	{198, "Page %1"},
	{200, "Chat"},
	{207, "Script\n\n\n\n\n\n"},
	{208, "Scripts (*.rbxs;*.lua;*.txt)|*.rbxs; *.lua; *.txt|All Files (*.*)|*.*||"},
	{9122, "\n&Add or Remove Buttons"},
	{9123, "\n&Reset Toolbar"},
	{9135, "\nPencil"},
	{9136, "\nFill Color"},
	{9137, "\nPick Color"},
	{9138, "\nLine"},
	{9139, "\nRectangle"},
	{9140, "\nEllipse"},
	{9141, "\nCopy"},
	{9142, "\nPaste"},
	{9143, "\nClear"},
	{9144, "\nUndo"},
	{9145, "\nRedo"},
	{9146, "\nImport from File"},
	{9160, "Close"},
	{9161, "Customize"},
	{9162, "New Menu"},
	{9163, "Custom %d"},
	{9164, "The toolbar name cannot be blank. Type a name."},
	{9166, "A toolbar named '%s' already exists. Type another name."},
	{9167, "Rename Toolbar"},
	{9168, "Are you sure you want to delete the '%s' toolbar?"},
	{9169, "All Commands"},
	{9170,
	 "This will delete the record of the commands you've used in this application and restore the default set of "
	 "visible commands to the menus and toolbars.  It will not undo any explicit customizations.   Are you sure you "
	 "want to do this?"},
	{9173, "Are you sure you want to reset the changes made to the '%s' toolbar?"},
	{9174, "Built-in Menus"},
	{9178,
	 "All Picture "
	 "Files|*.bmp;*.wmf;*.emf;*.ico;*.dib;*.cur;*.gif;*.jpg|Bitmaps(*.Bmp;*.Dib)|*.bmp;*.dib|Metafiles(*.Wmf;*.Emf)|*."
	 "wmf;*.emf|Icons(*.Ico;*.Cur)|*.ico;*.cur|GIF(*.Gif)|*.gif|JPEG(*.Jpg)|*.jpg|All Files(*.*)|*.*||"},
	{9179, "(System default)"},
	{9180, "Random"},
	{9181, "Unfold"},
	{9182, "Slide"},
	{9183, "Fade"},
	{9184, "None"},
	{32805, "Save the selection as a Model to a file"},
	{32807, "Pan Left\nPan Left"},
	{32808, "Pan Right\nPan Right"},
	{32809, "Tilt Up\nTilt Up"},
	{32810, "Tilt Down\nTilt Down"},
	{32811, "Zoom In\nZoom In"},
	{32812, "Zoom Out\nZoom Out"},
	{32813, "Zoom Extents\nZoom Extents"},
	{32825, "Group parts or models\nGroup"},
	{32826, "Ungroup models or parts\nUngroup"},
	{32832, "Fills a part with a color\nFill With Color"},
	{32833, "Picks up a color from a part\nPick Color"},
	{32835, "Rotate objects\nRotate objects"},
	{32836, "Tilt objects\nTilt Objects"},
	{32855, "Move objects up\nMove up"},
	{32856, "Move objects down\nMove down"},
	{32860, "Reset\nReset"},
	{32867, "Drag objects\nDrag"},
	{32869, "Resize Object\nResize"},
	{32871, "Move Objects\nMove on axis"},
	{32872, "Set velocity\nVelocity"},
	{32883, "Lock objects\nLock - prevent selection"},
	{32884, "Anchor objects\nAnchor - prevent moving"},
	{32887, "\nGlue Surface"},
	{32888, "\nSmooth Surface"},
	{32889, "\nWeld Surface"},
	{32891, "\nMotor on Surface"},
	{32892, "\nHinge on Surface"},
	{32934, "Center View on Object\nLook At Object"},
	{32935, "\nStuds"},
	{32936, "\nInlet"},
	{32950, "Save the active document to Roblox"},
	{32951, "Save the selection as a Model to Roblox"},
	{32965, "Play in Single Player\nPlaySolo"},
	{32966, "Test-run\nRun"},
	{32967, "Pauses the run\nPause"},
	{32968, "Stops and reloads the document\nReset"},
	{32969, "Address\nAddress"},
	{32970, "Go\nGo"},
	{35001, "Customize application appearance, toolbars, and menus\n&Customize..."},
	{57344, "Roblox"},
	{57345, "Ready"},
	{57600, "Create a new document\nNew"},
	{57601, "Open an existing document\nOpen"},
	{57602, "Close the active document\nClose"},
	{57603, "Save the active document\nSave"},
	{57604, "Save the active document with a new name\nSave As"},
	{57605, "Change the printing options\nPage Setup"},
	{57606, "Change the printer and printing options\nPrint Setup"},
	{57607, "Print the active document\nPrint"},
	{57609, "Display full pages\nPrint Preview"},
	{57616, "Open this document"},
	{57617, "Open this document"},
	{57618, "Open this document"},
	{57619, "Open this document"},
	{57620, "Open this document"},
	{57621, "Open this document"},
	{57622, "Open this document"},
	{57623, "Open this document"},
	{57624, "Open this document"},
	{57625, "Open this document"},
	{57626, "Open this document"},
	{57627, "Open this document"},
	{57628, "Open this document"},
	{57629, "Open this document"},
	{57630, "Open this document"},
	{57631, "Open this document"},
	{57632, "Erase the selection\nErase"},
	{57633, "Erase everything\nErase All"},
	{57634, "Copy the selection and put it on the Clipboard\nCopy"},
	{57635, "Cut the selection and put it on the Clipboard\nCut"},
	{57636, "Find the specified text\nFind"},
	{57637, "Insert Clipboard contents\nPaste"},
	{57640, "Repeat the last action\nRepeat"},
	{57641, "Replace specific text with different text\nReplace"},
	{57642, "Select the entire document\nSelect All"},
	{57643, "Undo the last action\nUndo"},
	{57644, "Redo the previously undone action\nRedo"},
	{57648, "Open another window for the active document\nNew Window"},
	{57649, "Arrange icons at the bottom of the window\nArrange Icons"},
	{57650, "Arrange windows so they overlap\nCascade Windows"},
	{57651, "Arrange windows as non-overlapping tiles\nTile Windows"},
	{57652, "Arrange windows as non-overlapping tiles\nTile Windows"},
	{57653, "Split the active window into panes\nSplit"},
	{57664, "Display program information, version number and copyright\nAbout"},
	{57665, "Quit the application; prompts to save documents\nExit"},
	{57680, "Switch to the next window pane\nNext Pane"},
	{57681, "Switch back to the previous window pane\nPrevious Pane"},
	{59136, "EXT"},
	{59137, "CAP"},
	{59138, "NUM"},
	{59139, "SCRL"},
	{59140, "OVR"},
	{59141, "REC"},
	{59392, "Show or hide the toolbar\nToggle ToolBar"},
	{59393, "Show or hide the status bar\nToggle StatusBar"},
	{61184, "Change the window size"},
	{61185, "Change the window position"},
	{61186, "Reduce the window to an icon"},
	{61187, "Enlarge the window to full size"},
	{61188, "Switch to the next document window"},
	{61189, "Switch to the previous document window"},
	{61190, "Close the active window and prompts to save the documents"},
	{61202, "Restore the window to normal size"},
	{61203, "Activate Task List"},
	{61215, "Activate this window"},
	{61440, "Open"},
	{61441, "Save As"},
	{61442, "All Files (*.*)"},
	{61443, "Untitled"},
	{61444, "Save Copy As"},
	{61445, "Close print preview mode\nCancel Preview"},
	{61505, "&One Page"},
	{61506, "&Two Page"},
	{61507, "Page %u"},
	{61508, "Page %u\nPages %u-%u\n"},
	{61509, "prn"},
	{61510, "Output.prn"},
	{61511, "Printer Files (*.prn)|*.prn|All Files (*.*)|*.*||"},
	{61512, "Print to File"},
	{61702, "Could not start print job."},
};

std::span<const MenuEntry> menu(unsigned id)
{
	switch (id) {
	case 103:
		return menu103;
	case 128:
		return menu128;
	case 129:
		return menu129;
	case 139:
		return menu139;
	case 140:
		return menu140;
	case 185:
		return menu185;
	case 186:
		return menu186;
	case 9121:
		return menu9121;
	}
	return {};
}

const DialogTemplate* dialog(unsigned id)
{
	for (const DialogTemplate& dialog : dialogs) {
		if (dialog.id == id) {
			return &dialog;
		}
	}
	return nullptr;
}

const ToolBar* toolBar(unsigned id)
{
	for (const ToolBar& bar : toolBars) {
		if (bar.id == id) {
			return &bar;
		}
	}
	return nullptr;
}

std::span<const Accelerator> accelerators(unsigned id)
{
	switch (id) {
	case 128:
		return accelerators128;
	case 207:
		return accelerators207;
	}
	return {};
}

QString string(unsigned id)
{
	auto it = strings.find(id);
	return it != strings.end() ? QString::fromLatin1(it->second) : QString();
}

QString bitmapPath(unsigned id)
{
	return QStringLiteral(":/res/bitmap_%1.bmp").arg(id);
}

static QString cursorPath(unsigned id)
{
	return QStringLiteral(":/res/cursor_%1.cur").arg(id);
}

QCursor cursor(unsigned id)
{
	QFile file(cursorPath(id));
	if (!file.open(QIODevice::ReadOnly)) {
		return QCursor();
	}
	QByteArray data = file.readAll();
	const uchar* bytes = reinterpret_cast<const uchar*>(data.constData());
	auto read16 = [bytes](int offset) { return qFromLittleEndian<quint16>(bytes + offset); };
	auto read32 = [bytes](int offset) { return qFromLittleEndian<qint32>(bytes + offset); };
	int hotX = read16(10);
	int hotY = read16(12);
	const uchar* dib = bytes + read32(18);
	int header = qFromLittleEndian<qint32>(dib);
	int width = qFromLittleEndian<qint32>(dib + 4);
	int height = qFromLittleEndian<qint32>(dib + 8) / 2;
	const uchar* palette = dib + header;
	bool whiteOne = palette[4] != 0;
	int stride = (width + 31) / 32 * 4;
	const uchar* xorBits = palette + 8;
	const uchar* andBits = xorBits + stride * height;
	QImage bitmap(width, height, QImage::Format_Mono);
	QImage mask(width, height, QImage::Format_Mono);
	for (int y = 0; y < height; y++) {
		int row = (height - 1 - y) * stride;
		for (int x = 0; x < width; x++) {
			bool white = ((xorBits[row + x / 8] >> (7 - x % 8)) & 1) == (whiteOne ? 1 : 0);
			bool transparent = (andBits[row + x / 8] >> (7 - x % 8)) & 1;
			bitmap.setPixel(x, y, transparent ? (white ? 1 : 0) : (white ? 0 : 1));
			mask.setPixel(x, y, transparent ? 0 : 1);
		}
	}
	return QCursor(QBitmap::fromImage(bitmap), QBitmap::fromImage(mask), hotX, hotY);
}

QString htmlPath(unsigned id)
{
	return QStringLiteral(":/res/html_%1.htm").arg(id);
}

QString iconPath(unsigned id)
{
	return QStringLiteral(":/res/icon_%1.ico").arg(id);
}

QString pngPath(const QString& name)
{
	return QStringLiteral(":/res/%1.png").arg(name.toLower());
}

} // namespace Resources
