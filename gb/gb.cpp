// gb.cpp : Defines the entry point for the application.
//

#include "stdafx.h"
#include "gb.h"
#include "Resource.h"
#include "CommCtrl.h"
#include "HttpUtils.h"
#include "SerialPort.h"
#include "DBUtils.cpp"
#include <stdlib.h>
#include "MD5.h"


// import json cpp
#pragma comment(lib, "json1.lib")
#include "json/json.h"
//

// import class User 
#include "users.h"

// file io
#include <fstream>
#include <iostream>

// import StringUtils
#include "StringUtils.h"

// tab control
#include <objidl.h>  
#include <gdiplus.h>  
using namespace Gdiplus;
//控件库以及相关宏  
#include <windowsx.h>//有HANDLE_MSG宏+标准控件功能函数宏  
#include <commctrl.h>//高级控件函数宏  
//依赖库  
//#pragma comment (lib,"user32.lib")  
//#pragma comment(lib, "comctl32.lib")  
//#pragma comment (lib,"gdi32.lib")  
//#pragma comment (lib,"gdiplus.lib")  
//#pragma comment (lib,"kernel32.lib")  
HWND Page[100];
int iPageIndex = 0;
LRESULT CALLBACK SearchProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);
LRESULT CALLBACK NoPayProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);
LRESULT CALLBACK PayedProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);



#pragma comment ( lib,"comctl32.lib" )  

#define MAX_LOADSTRING 100
#define ID_TIMER 1

// Global Variables:
HINSTANCE hInst;								// current instance
TCHAR szTitle[MAX_LOADSTRING];					// The title bar text
TCHAR szWindowClass[MAX_LOADSTRING];			// the main window class name

// dibang config
CSerialPort mySerialPort;
CSerialPort mySerialPort2;

// http config
string URL_LOGIN = "http://dibang.zz91.com/api/loginsave.html?username=<USERNAME>&pwd=<PASSWORD>";
string URL_GET_SELLER = "http://dibang.zz91.com/api/searchsuppliers.html?iccode=<iccode>";

// Forward declarations of functions included in this code module:
ATOM				MyRegisterClass(HINSTANCE hInstance);
BOOL				InitInstance(HINSTANCE, int);
LRESULT CALLBACK	WndProc(HWND, UINT, WPARAM, LPARAM);
INT_PTR CALLBACK	About(HWND, UINT, WPARAM, LPARAM);
INT_PTR CALLBACK	DialogStorage(HWND, UINT, WPARAM, LPARAM);
INT_PTR CALLBACK	DialogLogin(HWND, UINT, WPARAM, LPARAM);
void CALLBACK TimerSerialPort(HWND hwnd, UINT message, UINT iTimerID, DWORD dwTime); // 定时器的回调，显示地磅回传信息


int APIENTRY _tWinMain(HINSTANCE hInstance,
                     HINSTANCE hPrevInstance,
                     LPTSTR    lpCmdLine,
                     int       nCmdShow)
{
	UNREFERENCED_PARAMETER(hPrevInstance);
	UNREFERENCED_PARAMETER(lpCmdLine);

 	// TODO: Place code here.
	MSG msg;
	HACCEL hAccelTable;

	// Initialize global strings
	LoadString(hInstance, IDS_APP_TITLE, szTitle, MAX_LOADSTRING);
	LoadString(hInstance, IDC_GB, szWindowClass, MAX_LOADSTRING);
	MyRegisterClass(hInstance);

	// Perform application initialization:
	if (!InitInstance (hInstance, nCmdShow))
	{
		return FALSE;
	}

	hAccelTable = LoadAccelerators(hInstance, MAKEINTRESOURCE(IDC_GB));

	// Main message loop:
	while (GetMessage(&msg, NULL, 0, 0))
	{
		if (!TranslateAccelerator(msg.hwnd, hAccelTable, &msg))
		{
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
	}

	return (int) msg.wParam;
}

//
//  FUNCTION: MyRegisterClass()
//
//  PURPOSE: Registers the window class.
//
//  COMMENTS:
//
//    This function and its usage are only necessary if you want this code
//    to be compatible with Win32 systems prior to the 'RegisterClassEx'
//    function that was added to Windows 95. It is important to call this function
//    so that the application will get 'well formed' small icons associated
//    with it.
//
ATOM MyRegisterClass(HINSTANCE hInstance)
{
	WNDCLASSEX wcex;

	wcex.cbSize = sizeof(WNDCLASSEX);

	wcex.style			= CS_HREDRAW | CS_VREDRAW;
	wcex.lpfnWndProc	= WndProc;
	wcex.cbClsExtra		= 0;
	wcex.cbWndExtra		= 0;
	wcex.hInstance		= hInstance;
	wcex.hIcon			= LoadIcon(hInstance, MAKEINTRESOURCE(IDI_GB));
	wcex.hCursor		= LoadCursor(NULL, IDC_ARROW);
	wcex.hbrBackground	= (HBRUSH)(COLOR_WINDOW+1);
	wcex.lpszMenuName	= MAKEINTRESOURCE(IDC_GB);
	wcex.lpszClassName	= szWindowClass;
	wcex.hIconSm		= LoadIcon(wcex.hInstance, MAKEINTRESOURCE(IDI_SMALL));

	return RegisterClassEx(&wcex);
}

//
//   FUNCTION: InitInstance(HINSTANCE, int)
//
//   PURPOSE: Saves instance handle and creates main window
//
//   COMMENTS:
//
//        In this function, we save the instance handle in a global variable and
//        create and display the main program window.
//
BOOL InitInstance(HINSTANCE hInstance, int nCmdShow)
{
  HWND hWnd;

   hInst = hInstance; // Store instance handle in our global variable

   hWnd = CreateWindow(szWindowClass, szTitle, WS_OVERLAPPEDWINDOW,CW_USEDEFAULT, 0, CW_USEDEFAULT, 0, NULL, NULL, hInstance, NULL);
  // hWnd = CreateDialog(hInstance, MAKEINTRESOURCE(IDD_DIALOG_STORAGE), GetDesktopWindow(), (DLGPROC)DialogStorage);

   if (!hWnd)
   {
      return FALSE;
   }

   ShowWindow(hWnd, nCmdShow);
   UpdateWindow(hWnd);
   return TRUE;
}


//
//  FUNCTION: WndProc(HWND, UINT, WPARAM, LPARAM)
//
//  PURPOSE:  Processes messages for the main window.
//
//  WM_COMMAND	- process the application menu
//  WM_PAINT	- Paint the main window
//  WM_DESTROY	- post a quit message and return
//
//
// 

LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	int wmId, wmEvent;
	PAINTSTRUCT ps;
	HDC hdc;
	char clientid[50];
	ifstream infile;
	DBUtils db;
	char sql[300];
	string sqlString;
	int count = 0;
	HWND hBn;
	HWND hDialog;
	
	switch (message)
	{
	case WM_CREATE:
		/*初始化线程实时同步磅秤信息*/
		if (!mySerialPort.InitPort(3, CBR_1200, 'N', 8, 1, EV_RXCHAR))
		{
			MessageBox(NULL, TEXT("设备1(com3口)异常，请检查后重试！"), TEXT("消息"), 0);
		}
		else
		{
			MessageBox(NULL, TEXT("设备1(com3口)启动正常！"), TEXT("消息"), 0);
		}
		/*if (!mySerialPort2.InitPort(1, CBR_1200, 'N', 8, 1, EV_RXCHAR))
		{
			MessageBox(NULL, TEXT("设备2异常，请检查后重试！"), TEXT("消息"), 0);
		}
		else
		{
			MessageBox(NULL, TEXT("设备2正常！"), TEXT("消息"), 0);
		}*/
		hBn = GetDlgItem(hWnd, IDOK);
		SetWindowLong(hBn, GWL_STYLE, GetWindowLong(hBn, GWL_STYLE) | BS_OWNERDRAW);
		//判定是否有client，如果有，则获取client下的用户登录
		infile.open("client.config");
		infile >> clientid;
		sqlString = "select id from users where clientid = '<CLIENTID>'";
		sqlString = sqlString.replace(sqlString.find("<CLIENTID>"),10,clientid);
		strcpy(sql, sqlString.c_str());
		db.getRecordRet(sql);
		
		while (!db.HX_pRecordset->adoEOF) {
			_variant_t var;
			string strValue;
			var = db.HX_pRecordset->GetCollect("id");
			if (var.vt != VT_NULL) {
				strValue = _com_util::ConvertBSTRToString((_bstr_t)var);
			}
			count = atoi(strValue.c_str());
			db.HX_pRecordset->MoveNext();
		}
		db.ExitConnect();
		if (count>0)
		{
			//MessageBox(NULL, TEXT("登陆成功"), TEXT("消息"), 0);
			DialogBox(hInst, MAKEINTRESOURCE(IDD_DIALOG_STORAGE), hWnd, DialogStorage);
		}
		else {
			DialogBox(hInst, MAKEINTRESOURCE(IDD_DIALOG_LOGIN), hWnd, DialogLogin);
		}
        break;
	case WM_COMMAND:
		wmId    = LOWORD(wParam);
		wmEvent = HIWORD(wParam);
		// Parse the menu selections:
		switch (wmId)
		{
		case IDM_ABOUT:
			//MessageBox(NULL,TEXT("关于"),TEXT("lololo"),0);
			DialogBox(hInst, MAKEINTRESOURCE(IDD_ABOUTBOX), hWnd, About);
			break;
		case IDM_EXIT:
		//	MessageBox(NULL,TEXT("关闭咯！"),TEXT("lololo "),0);
			DestroyWindow(hWnd);
			break;
		case IDM_STORAGE:
			//DialogBox(hInst, MAKEINTRESOURCE(IDD_DIALOG_STORAGE), hWnd, DialogStorage);
			hDialog = CreateDialog(hInst, MAKEINTRESOURCE(IDD_DIALOG_STORAGE), hWnd, DialogStorage);
			ShowWindow(hDialog, SW_SHOW);
			break;
		case IDM_LOGIN:
			DialogBox(hInst, MAKEINTRESOURCE(IDD_DIALOG_LOGIN), hWnd, DialogLogin);
			break;
		default:
			return DefWindowProc(hWnd, message, wParam, lParam);
		}
		break;
	case WM_PAINT:
		hdc=BeginPaint( hWnd,&ps ); //取得设备环境句柄
		EndPaint(hWnd, &ps);
		break;
	case WM_DESTROY:
		PostQuitMessage(0);
		break;
	default:
		return DefWindowProc(hWnd, message, wParam, lParam);
	}
	return 0;
}

// Message handler for about box.
INT_PTR CALLBACK About(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
	UNREFERENCED_PARAMETER(lParam);
	switch (message)
	{
	case WM_INITDIALOG:
		return (INT_PTR)TRUE;

	case WM_COMMAND:
		if (LOWORD(wParam) == IDOK || LOWORD(wParam) == IDCANCEL)
		{
			EndDialog(hDlg, LOWORD(wParam));
			return (INT_PTR)TRUE;
		}
		break;
	}
	return (INT_PTR)FALSE;
}

void initTableUser(HWND hDlg) {
	HWND hListview;
//	hListview = GetDlgItem(hDlg, IDC_LIST_USER);
	// 设置ListView的列
	LVCOLUMN vcl;
	vcl.mask = LVCF_TEXT | LVCF_WIDTH | LVCF_SUBITEM;
	vcl.pszText = "序号";//列标题
	vcl.cx = 40;//列宽
	vcl.iSubItem = 0;//子项索引，第一列无子项
	ListView_InsertColumn(hListview, 0, &vcl);
	vcl.mask = LVCF_TEXT | LVCF_WIDTH | LVCF_SUBITEM;
	vcl.pszText = "供货人编号";
	vcl.cx = 90;
	vcl.iSubItem = 1;//子项索引
	ListView_InsertColumn(hListview, 1, &vcl);
	vcl.pszText = "供货人";
	vcl.cx = 60;
	vcl.iSubItem = 2;
	ListView_InsertColumn(hListview, 2, &vcl);
	vcl.pszText = "联系电话";
	vcl.cx = 60;
	vcl.iSubItem = 3;
	ListView_InsertColumn(hListview, 3, &vcl);
	vcl.pszText = "供应商类型";
	vcl.cx = 80;
	vcl.iSubItem = 4;
	ListView_InsertColumn(hListview, 4, &vcl);
	vcl.pszText = "地址";
	vcl.cx = 60;
	vcl.iSubItem = 5;
	ListView_InsertColumn(hListview, 5, &vcl);
}

void initHeadUser(HWND hListview) {
	// 设置ListView的列
	LVCOLUMN vcl;
	vcl.mask = LVCF_TEXT | LVCF_WIDTH | LVCF_SUBITEM;
	vcl.pszText = "序号";//列标题
	vcl.cx = 40;//列宽
	vcl.iSubItem = 0;//子项索引，第一列无子项
	ListView_InsertColumn(hListview, 0, &vcl);
	vcl.mask = LVCF_TEXT | LVCF_WIDTH | LVCF_SUBITEM;
	vcl.pszText = "供货人编号";
	vcl.cx = 90;
	vcl.iSubItem = 1;//子项索引
	ListView_InsertColumn(hListview, 1, &vcl);
	vcl.pszText = "供货人";
	vcl.cx = 60;
	vcl.iSubItem = 2;
	ListView_InsertColumn(hListview, 2, &vcl);
	vcl.pszText = "联系电话";
	vcl.cx = 60;
	vcl.iSubItem = 3;
	ListView_InsertColumn(hListview, 3, &vcl);
	vcl.pszText = "供应商类型";
	vcl.cx = 80;
	vcl.iSubItem = 4;
	ListView_InsertColumn(hListview, 4, &vcl);
	vcl.pszText = "地址";
	vcl.cx = 60;
	vcl.iSubItem = 5;
	ListView_InsertColumn(hListview, 5, &vcl);
}

void initTableStorage(HWND hDlg){
	HWND hListview;
	string httpResult;
	LVITEM lvItem;
	queue<DBInfo> list;
	int queueSize;
	int i;
	hListview = GetDlgItem(hDlg, IDC_LIST_STORAGE);
	// 设置ListView的列
	LVCOLUMN vcl;
	vcl.mask = LVCF_TEXT | LVCF_WIDTH | LVCF_SUBITEM;
	vcl.pszText = "序号";//列标题
	vcl.cx = 40;//列宽
	vcl.iSubItem = 0;//子项索引，第一列无子项
	ListView_InsertColumn(hListview, 0, &vcl);
	vcl.mask = LVCF_TEXT | LVCF_WIDTH | LVCF_SUBITEM;
	vcl.pszText = "供货人编号";
	vcl.cx = 90;
	vcl.iSubItem = 1;//子项索引
	ListView_InsertColumn(hListview, 1, &vcl);
	vcl.pszText = "供货人";
	vcl.cx = 60;
	vcl.iSubItem = 2;
	ListView_InsertColumn(hListview, 2, &vcl);
	vcl.pszText = "进场时间";
	vcl.cx = 60;
	vcl.iSubItem = 3;
	ListView_InsertColumn(hListview, 3, &vcl);
	vcl.pszText = "毛重";
	vcl.cx = 50;
	vcl.iSubItem = 4;
	ListView_InsertColumn(hListview, 4, &vcl);
	vcl.pszText = "皮重";
	vcl.cx = 60;
	vcl.iSubItem = 5;
	ListView_InsertColumn(hListview, 5, &vcl);
	vcl.pszText = "品名";
	vcl.cx = 50;
	vcl.iSubItem = 6;
	ListView_InsertColumn(hListview, 6, &vcl);
	vcl.pszText = "单价";
	vcl.cx = 90;
	vcl.iSubItem = 7;
	ListView_InsertColumn(hListview, 7, &vcl);
	vcl.pszText = "净重";
	vcl.cx = 90;
	vcl.iSubItem = 8;
	ListView_InsertColumn(hListview, 8, &vcl);
	vcl.pszText = "总额";
	vcl.cx = 90;
	vcl.iSubItem = 9;
	ListView_InsertColumn(hListview, 9, &vcl);
	vcl.pszText = "状态";
	vcl.cx = 90;
	vcl.iSubItem = 10;
	ListView_InsertColumn(hListview, 10, &vcl);

	HttpUtils hu ;
	httpResult = hu.httpGet(_T("http://sym.zz91.com/fragment/esite/newsList.htm?cid=123516472&left2017md1"));		
	list = hu.ParseJsonInfoForList(httpResult);
	queueSize = list.size();
	lvItem.mask = LVIF_TEXT; 
	for(i=0;i<queueSize;i++){
		char str[100];
        lvItem.iItem = i;				// 行
        lvItem.iSubItem = 0;			// 列1
		lvItem.pszText = "abc";			// 内容
        ListView_InsertItem(hListview, (LPARAM)&lvItem);  
		// 列2
		lvItem.iSubItem = 1;			
		sprintf(str,"%d",list.front().getCId());	
		lvItem.pszText = str;
        ListView_SetItem( hListview, (LPARAM)&lvItem);
		// TODO

		list.pop();						// 下一个队列元素
	}
}

HWND CreatePageWnd(HWND hTab, WNDPROC PageWndProc)
{
	RECT rcTabDisplay;
	//获取Tab控件客户区的的大小（Tab窗口坐标系）  
	GetClientRect(hTab, &rcTabDisplay);//获取Tab窗口的大小（而不是在屏幕坐标系中）  
	TabCtrl_AdjustRect(hTab, FALSE, &rcTabDisplay);//通过窗口大小，获取客户区大小。  
												   //创建与Tab控件客户区大小的画布  
	HWND hPage = CreateWindowEx(
		WS_EX_WINDOWEDGE,       //window extend Style  
		WC_LISTVIEW,   // window class name  
		NULL,  // window caption  
		WS_CHILD|LVS_REPORT|WS_BORDER | WS_TABSTOP,//WS_TABSTOP |  |  WS_VISIBLE |      // window style  
		rcTabDisplay.left,            // initial x position  
		rcTabDisplay.top,            // initial y position  
		rcTabDisplay.right,            // initial x size  
		rcTabDisplay.bottom,            // initial y size  
		hTab,                     // parent window handle  
		NULL,                     // window menu handle  
		hInst,                // program instance handle  
		NULL);                    // creation parameters  
//	SetWindowLongPtr(hPage, GWLP_WNDPROC, (LONG_PTR)PageWndProc);//子类化窗口
	// 扩展风格添加 start 
	//整行选中 网格线 添加checkbox 三项
	DWORD styles = LVS_EX_FULLROWSELECT | LVS_EX_GRIDLINES | LVS_EX_CHECKBOXES;
	ListView_SetExtendedListViewStyleEx(hPage, styles,styles);
	// 扩展风格 end
	initHeadUser(hPage);// 初始化表格头
	return hPage;
}


//功能:向Tab中添加标签  
int InsertTabItem(HWND hTab, LPTSTR pszText, int iid)
{
	TCITEM ti = { 0 };
	ti.mask = TCIF_TEXT;
	ti.pszText = pszText;
	ti.cchTextMax = strlen(pszText);
	return (int)SendMessage(hTab, TCM_INSERTITEM, iid, (LPARAM)&ti);
}

/*
功能：向Tab中添加页面
实现原理：一个窗口多个窗口处理函数，每个窗口处理函数负责一个页面的布局和处理
注意：该添加方式是顺序添加，即AddAfter，在当前页面后添加
*/
BOOL addPage(HWND hTab, LPTSTR strPageLabel, WNDPROC PageWndProc)
{
	InsertTabItem(hTab, strPageLabel, iPageIndex);
	HWND hPage = CreatePageWnd(hTab, PageWndProc);
	Page[iPageIndex++] = hPage;
	return TRUE;
}

//Page自己的窗口类，通过SetWindowLongPtr（）可以子类化  
ATOM RegisterPageClass()
{
	WNDCLASSEX wcex;
	wcex.cbSize = sizeof(WNDCLASSEX);
	wcex.style = CS_HREDRAW | CS_VREDRAW;
	wcex.cbClsExtra = 0;
	wcex.cbWndExtra = 0;
	wcex.hInstance = hInst;
	wcex.hIcon = NULL;
	wcex.hIconSm = NULL;
	wcex.lpszMenuName = NULL;
	wcex.hCursor = LoadCursor(NULL, IDC_ARROW);
	wcex.hbrBackground = (HBRUSH)GetStockObject(WHITE_BRUSH);
	wcex.lpszClassName = TEXT("Page");
	wcex.lpfnWndProc = DefWindowProc;
	return RegisterClassEx(&wcex);
}

/*功能：初始化所有页面*/
BOOL InitPages(HWND hTab)
{
	
	return TRUE;
}

#pragma endregion  
#pragma region TabControl  
HWND InitTabControl(HWND hParentWnd, HWND hWndFocus, LPARAM lParam)
{
	INITCOMMONCONTROLSEX iccx;
	iccx.dwSize = sizeof(INITCOMMONCONTROLSEX);
	iccx.dwICC = ICC_TAB_CLASSES;
	if (!InitCommonControlsEx(&iccx))
		return FALSE;
	RECT rc;
	GetClientRect(hParentWnd, &rc);
	// Set the font of the tabs to a more typical system GUI font  
	SendMessage(hParentWnd, WM_SETFONT, (WPARAM)GetStockObject(DEFAULT_GUI_FONT), 0);
	//根据页面来初始化页面  
	RegisterPageClass();
	addPage(hParentWnd, TEXT("查询结果"), SearchProc);
	addPage(hParentWnd, TEXT("未结算供应商"), NoPayProc);
	addPage(hParentWnd, TEXT("已结算供应商"), PayedProc);
	ShowWindow(Page[0], SW_SHOW);//显示第一个页面  

	LVITEM lvItem;
	lvItem.mask = LVIF_TEXT;
	char str[100];
	lvItem.iItem = 0;				// 行
	lvItem.iSubItem = 0;			// 列1
	lvItem.pszText = "abc";			// 内容
	ListView_InsertItem(Page[0], (LPARAM)&lvItem);
	// 列2
	lvItem.iSubItem = 1;
	sprintf(str, "%d", 123);
	lvItem.pszText = str;
	ListView_SetItem(Page[0], (LPARAM)&lvItem);

	return hParentWnd;
}

// Message handler for about box.
INT_PTR CALLBACK DialogStorage(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
	
	/* 初始化磅秤句柄  */
	//CSerialPort mySerialPort;
	//mySerialPort.setHDlg(hDlg);
	char str[100];
	HDC hdc;
	hdc = GetDC(hDlg);
	PAINTSTRUCT ps;

	HWND hbn = (HWND)lParam;
	RECT rc;
	TCHAR text[64];
	HWND hBn;
	int nWidth,nHeight;
	//
	LPDRAWITEMSTRUCT pdis;
	HDC hMemDc;
	HFONT   hFont;

	HWND hTab;
	int sel

	UNREFERENCED_PARAMETER(lParam);
	switch (message)
	{
	case WM_INITDIALOG:
		// 定时器
		SetTimer(hDlg, ID_TIMER, 2000, TimerSerialPort);

		// 按钮自定义样式
		hBn = GetDlgItem(hDlg, IDC_BUTTON_PAY);
		SetWindowLong(hBn, GWL_STYLE, GetWindowLong(hBn, GWL_STYLE) | BS_OWNERDRAW);
		hBn = GetDlgItem(hDlg, IDC_BUTTON_SET_MONEY);
		SetWindowLong(hBn, GWL_STYLE, GetWindowLong(hBn, GWL_STYLE) | BS_OWNERDRAW);

		initTableStorage(hDlg); //入库单表格初始化

		////////////////////////////////////////////////////////////  显示tab标签
		iPageIndex = 0;
		hTab = GetDlgItem(hDlg, IDC_TAB1);
		InitTabControl(hTab, NULL, lParam);//主窗口创建的时候，同时创建Tab子窗口  

		//////////////////////////////////////////////////////////////  
		return (INT_PTR)TRUE;
	case WM_COMMAND:
		if (LOWORD(wParam) == IDOK || LOWORD(wParam) == IDCANCEL)
		{
			EndDialog(hDlg, LOWORD(wParam));
			return (INT_PTR)TRUE;
		}
		if (LOWORD(wParam) == IDC_BUTTON_PAY)
		{
			return (INT_PTR)TRUE;
		}
		break;
	case WM_PAINT:
		hdc = BeginPaint(hDlg, &ps); //取得设备环境句柄
		// 结算金额初始化
		LOGFONT   logfont;       //改变输出字体
		ZeroMemory(&logfont, sizeof(LOGFONT));
		logfont.lfCharSet = GB2312_CHARSET;
		logfont.lfHeight = -28;      //设置字体的大小
		hFont = CreateFontIndirect(&logfont);
		SetBkColor(hdc, RGB(240, 240, 240));
		////SetBkMode(hdc, TRANSPARENT); 透明样式，没用。适合固定展示，不适合经常变动的数据
		SelectObject(hdc, hFont);
		char zsBuffer[40];
		TextOut(hdc, 500, 540, zsBuffer, sprintf(zsBuffer, TEXT("结算金额:")));
		SetTextColor(hdc, RGB(255, 140, 102)); // 颜色
		TextOut(hdc, 635, 540, zsBuffer, sprintf(zsBuffer, TEXT("0元"))); 
		ReleaseDC(hDlg, hdc);
		DeleteObject(hFont);
		EndPaint(hDlg, &ps);
		break;
	case WM_CTLCOLORBTN://设置按钮的颜色
		if ((HWND)lParam == GetDlgItem(hDlg, IDC_BUTTON_PAY))
		{
			GetWindowText(hbn, text, 23);
			GetClientRect(hbn, &rc);
			SetTextColor(hdc, RGB(255, 255, 255));//设置按钮上文本的颜色
			SetBkMode(hdc, TRANSPARENT);
			rc.left = 195;
			rc.top = 541;
			rc.right = 287;
			rc.bottom = 606;
			MoveToEx(hdc,150,150,NULL);
			DrawText(hdc, text, _tcslen(text), &rc, DT_CENTER | DT_VCENTER | DT_SINGLELINE|DT_BOTTOM|WS_EX_TRANSPARENT);
			return (INT_PTR)CreateSolidBrush(RGB(255, 140, 102));//返回画刷设置按钮的背景色
		}
		if ((HWND)lParam == GetDlgItem(hDlg, IDC_BUTTON_SET_MONEY))
		{
			GetWindowText(hbn, text, 23);
			GetClientRect(hbn, &rc);
			SetTextColor(hdc, RGB(255, 255, 255));//设置按钮上文本的颜色
			SetBkMode(hdc, TRANSPARENT);
			rc.left = 810;
			rc.top = 541;
			rc.right = 900;
			rc.bottom = 606;
			MoveToEx(hdc, 150, 150, NULL);
			DrawText(hdc, text, _tcslen(text), &rc, DT_CENTER | DT_VCENTER | DT_SINGLELINE | DT_BOTTOM | WS_EX_TRANSPARENT);
			return (INT_PTR)CreateSolidBrush(RGB(0, 180, 240));//返回画刷设置按钮的背景色
		}
		break;
	case WM_DRAWITEM:
		//获得绘制结构体，包含绘制的按钮DC和当前按钮状态等  
		if (LOWORD(wParam) == IDC_BUTTON_PAY) {
			pdis = (LPDRAWITEMSTRUCT)lParam;
			if (pdis->CtlType == ODT_BUTTON)//只绘制button类型  
			{
				hdc = pdis->hDC;
				RECT itemRect = pdis->rcItem;
				SaveDC(hdc);//保存DC,绘制完必须恢复默认  
							//绘制默认状态  
				hMemDc = CreateCompatibleDC(hdc);
				//SelectObject(hMemDc, pdis->CtlID == IDOK ? hBitmapOK_U : hBitmapCANCEL_U);
				//BitBlt(hdc, 0, 0, bm.bmWidth, bm.bmHeight, hMemDc, 0, 0, SRCCOPY);
				DeleteDC(hMemDc);
				//绘制获取焦点时状态  
				if (pdis->itemState & ODS_FOCUS)
				{
					HBRUSH br = CreateSolidBrush(RGB(255, 255, 255));
					FrameRect(hdc, &itemRect, br);
					InflateRect(&itemRect, -1, -1);
					DeleteObject(br);
				}
				//绘制下压状态  
				if (pdis->itemState & ODS_SELECTED)
				{
					HBRUSH brBtnShadow = CreateSolidBrush(GetSysColor(COLOR_BTNSHADOW));
					FrameRect(hdc, &itemRect, brBtnShadow);
					DeleteObject(brBtnShadow);
				}
				else {
					UINT uState = DFCS_BUTTONPUSH |
						((pdis->itemState & ODS_SELECTED) ? DFCS_PUSHED : 0);
					HBRUSH br = CreateSolidBrush(RGB(194, 194, 194));
					FrameRect(hdc, &itemRect, br);
					InflateRect(&itemRect, -1, -1);
					DeleteObject(br);
					//DrawFrameControl(hdc, &itemRect, DFC_BUTTON, uState);
					//CreateSolidBrush(RGB(255, 140, 102));
				}
				RestoreDC(hdc, -1);
			}
		}
		if (LOWORD(wParam) == IDC_BUTTON_SET_MONEY) {
			pdis = (LPDRAWITEMSTRUCT)lParam;
			if (pdis->CtlType == ODT_BUTTON)//只绘制button类型  
			{
				hdc = pdis->hDC;
				RECT itemRect = pdis->rcItem;
				SaveDC(hdc);//保存DC,绘制完必须恢复默认  
							//绘制默认状态  
				hMemDc = CreateCompatibleDC(hdc);
				//SelectObject(hMemDc, pdis->CtlID == IDOK ? hBitmapOK_U : hBitmapCANCEL_U);
				//BitBlt(hdc, 0, 0, bm.bmWidth, bm.bmHeight, hMemDc, 0, 0, SRCCOPY);
				DeleteDC(hMemDc);
				//绘制获取焦点时状态  
				if (pdis->itemState & ODS_FOCUS)
				{
					HBRUSH br = CreateSolidBrush(RGB(255, 255, 255));
					FrameRect(hdc, &itemRect, br);
					InflateRect(&itemRect, -1, -1);
					DeleteObject(br);
				}
				//绘制下压状态  
				if (pdis->itemState & ODS_SELECTED)
				{
					HBRUSH brBtnShadow = CreateSolidBrush(GetSysColor(COLOR_BTNSHADOW));
					FrameRect(hdc, &itemRect, brBtnShadow);
					DeleteObject(brBtnShadow);
				}
				else {
					UINT uState = DFCS_BUTTONPUSH |
						((pdis->itemState & ODS_SELECTED) ? DFCS_PUSHED : 0);
					HBRUSH br = CreateSolidBrush(RGB(194, 194, 194));
					FrameRect(hdc, &itemRect, br);
					InflateRect(&itemRect, -1, -1);
					DeleteObject(br);
					//DrawFrameControl(hdc, &itemRect, DFC_BUTTON, uState);
					//CreateSolidBrush(RGB(255, 140, 102));
				}
				RestoreDC(hdc, -1);
			}
		}
		return (TRUE);
	case WM_SIZE:
		nWidth = LOWORD(lParam);
		nHeight = LOWORD(wParam);
		return (INT_PTR)FALSE;
	case WM_NOTIFY:      //TAB控件切换发生时传送的消息  
		hTab = GetDlgItem(hDlg, IDC_TAB1);
		if ((INT)wParam == IDC_TAB1) //这里也可以用一个NMHDR *nm = (NMHDR *)lParam这个指针来获取 句柄和事件  
		{                   //读者可自行查找NMHDR结构  
			if (((LPNMHDR)lParam)->code == TCN_SELCHANGE) //当TAB标签转换的时候发送TCN_SELCHANGE消息  
			{
				int sel = TabCtrl_GetCurSel(hTab);
				switch (sel)   //根据索引值查找相应的标签值，干相应的事情  
				{

				case 0: //TAB1标签时，我们要显示 tab1标签页面  
				{
					ShowWindow(Page[0], TRUE); //显示窗口用ShowWindow函数  
					ShowWindow(Page[1], FALSE);
					ShowWindow(Page[2], FALSE);
					break;
				}
				case 1://TAB2标签时，我们要显示 tab2标签页面  
				{
					ShowWindow(Page[0], FALSE);
					ShowWindow(Page[1], TRUE);
					ShowWindow(Page[2], FALSE);
					break;
				}
				case 2://TAB2标签时，我们要显示 tab3标签页面  
				{
					ShowWindow(Page[0], FALSE);
					ShowWindow(Page[1], FALSE);
					ShowWindow(Page[2], TRUE);
					break;
				}
				}
			}
		}
		return (INT_PTR)FALSE;
	case WM_CLOSE:
		KillTimer(hDlg, ID_TIMER);  // 关闭线程
		return 0;
	}
	
	return (INT_PTR)FALSE;
}

// Message handler for about box.
INT_PTR CALLBACK DialogLogin(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{

	UNREFERENCED_PARAMETER(lParam);
	switch (message)
	{
	case WM_INITDIALOG:
		return (INT_PTR)TRUE;
	case WM_COMMAND:
		if (LOWORD(wParam) == IDOK || LOWORD(wParam) == IDCANCEL)
		{
			EndDialog(hDlg, LOWORD(wParam));
			return (INT_PTR)TRUE;
		}
		if (LOWORD(wParam) == IDC_DO_LOGIN )   // 登录逻辑
		{
			char clientid[50];		
			memset(clientid, 0, 50);
			HWND accounthDlg = GetDlgItem(hDlg, IDC_ACCOUNT);
			HWND pwdDlg = GetDlgItem(hDlg, IDC_PASSWORD);
			char username[50];					// 获取输入用户名
			char password[50];					// 获取密码
			char md5Pwd[50];					// md5 加密过后的密码 16位		
			string md5PwdString;
			string tempMd5;
			GetWindowText(accounthDlg, username, 50);
			GetWindowText(pwdDlg, password, 50);
			MD5 md5;
			md5PwdString = password;
			tempMd5 = md5.Encode(md5PwdString).substr(8,16);
			strcpy(md5Pwd, tempMd5.c_str());
			// 完整一次sql-----start
			DBUtils dbUtils;
			string sqlString = "select count(*) as hasExist from users where username='<USERNAME>' and pwd='<PWD>' ";
			sqlString = sqlString.replace(sqlString.find("<USERNAME>"),10,username);
			sqlString = sqlString.replace(sqlString.find("<PWD>"), 5, md5PwdString);
			char sql[300];
			memset(sql, 0, 300);
			strcpy(sql,sqlString.c_str());
			dbUtils.getRecordRet(sql);
			int count;
			while (!dbUtils.HX_pRecordset->adoEOF) {
				_variant_t var;
				string strValue;
				var  = dbUtils.HX_pRecordset->GetCollect("hasExist");
				if (var.vt != VT_NULL)
				strValue = _com_util::ConvertBSTRToString((_bstr_t)var);
				count = atoi(strValue.c_str()); 
				dbUtils.HX_pRecordset->MoveNext();
			}
			dbUtils.ExitConnect();// 关闭连接
			// 完整一次sql----end

			// 本地不存在数据 走http外网
			// 添加inc文件 当前目录下client.config
			if (count <= 0) {
				// 当前时间获取
				time_t now = time(NULL);
				char* nowChar = asctime(gmtime(&now));
				string temp = nowChar;
				string clientidstring = temp;
				temp = username;
				clientidstring = clientidstring + temp;
				clientidstring = md5.Encode(clientidstring).substr(8,16);
				strcpy(clientid, clientidstring.c_str());
				// 打开文件
				ofstream outfile;
				outfile.open("client.config");
				outfile << clientid << endl;		//输入文本
				outfile.close();					// 文件操作结束

				char httpUrl[100];
				string url = URL_LOGIN;
				url = url.replace(url.find("<USERNAME>"),10,username);
				url = url.replace(url.find("<PASSWORD>"), 10, password);
				strcpy(httpUrl, url.c_str());
				HttpUtils hu;
				string httpResult = hu.httpGet(_T(httpUrl));
				Json::Reader reader;                                    //解析json用Json::Reader
				Json::Value value;
				//可以代表任意类型
				if (!reader.parse(httpResult, value))
				{
					// 检验失败，弹框返回返回
					MessageBox(NULL, TEXT("网络故障请重试！"), TEXT("消息"), 0);
				}
				else {
					char selfidChar[50];
					if (value["err"].asString()=="true") {
						char errMsg[50];
						string errMsgString = value["errtext"].asString();
						strcpy(errMsg,errMsgString.c_str());
						MessageBox(NULL, TEXT(errMsg), TEXT("消息"), 0);
					}
					else {
						// 检验成功，导入数据进本地数据库
						string selfid = value["result"]["selfid"].asString();
						strcpy(selfidChar, selfid.c_str());
						int group_id = value["result"]["group_id"].asInt();
						int company_id = value["result"]["company_id"].asInt();
						string sqlInsert = "Insert INTO users (selfid,group_id,company_id,clientid,username,pwd) VALUES ('<SELFID>',<GROUP_ID>,<COMPANY_ID>,'<CLIENTID>','<USERNAME>','<PWD>')";
						sqlInsert.replace(sqlInsert.find("<SELFID>"),8, selfidChar);
						char groupArray[11];
						sqlInsert.replace(sqlInsert.find("<GROUP_ID>"), 10, itoa(group_id,groupArray,10));
						char companyArray[11];
						sqlInsert.replace(sqlInsert.find("<COMPANY_ID>"), 12, itoa(company_id, companyArray, 10));
						sqlInsert.replace(sqlInsert.find("<CLIENTID>"), 10, clientid);
						sqlInsert.replace(sqlInsert.find("<USERNAME>"),10,username);
						sqlInsert.replace(sqlInsert.find("<PWD>"),5, md5Pwd);
						char sql[1000];
						strcpy(sql,sqlInsert.c_str());
						dbUtils.addRecord(sql);
						EndDialog(hDlg, LOWORD(wParam));
					}
				}
			}

			return (INT_PTR)TRUE;
		}
		
		break;
	}
	return (INT_PTR)FALSE;
}

void CALLBACK TimerSerialPort(HWND hwnd,UINT message ,UINT iTimerID,DWORD dwTime) {
	HDC hdc = GetDC(hwnd);
	char result[100];
	mySerialPort.Read(result);
	int i = 0;
	for (; i < 100; i++) {
		if (result[i] == '\r') {
			break;
		}
	}
	char out[100] = "00000.0kg";
	memset(out, '\0', 100);
	if (i >= 5) {
		for (int j = 0, k = 0; j < i; j++) {
			if (result[j] == '\n' || result[j] == 'w' || result[j] == 'n') {
				continue;
			}
			out[k] = result[j];
			k++;
		}
	}
	int xi = 0;
	for (; xi < 12; xi++) {
		if (out[xi] == '.') {
			break;
		}
	}
	if (xi < 5) {
		char zero[5] = "";
		memset(out, '\0', 5);
		int xZero = 0;
		for (; xi <= 5 - xi; xi++) {
			zero[xZero] = '0';
			xZero++;
		}

		sprintf(out, "%s%s", zero, out);
	}
	LOGFONT   logfont;       //改变输出字体
	ZeroMemory(&logfont, sizeof(LOGFONT));
	logfont.lfCharSet = GB2312_CHARSET;
	logfont.lfHeight = -28;      //设置字体的大小
	HFONT   hFont = CreateFontIndirect(&logfont);

//	HDC hdc = ::GetDC(m_hWnd);
	SetTextColor(hdc, RGB(230, 57, 0)); // 颜色
	SetBkColor(hdc, RGB(240, 240, 240));
	////SetBkMode(hdc, TRANSPARENT);
	SelectObject(hdc, hFont);
	TextOut(hdc, 28, 77, out, strlen(out));
	ReleaseDC(hwnd, hdc);
	DeleteObject(hFont);
	MessageBeep(-1);
}

LRESULT CALLBACK SearchProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	HDC          hdc;
	PAINTSTRUCT  ps;
	HWND hDialog;
	switch (message)
	{
	case WM_PAINT:
	{
		hdc = BeginPaint(hWnd, &ps);
		EndPaint(hWnd, &ps);
	}
	return 0;
	default:
		return DefWindowProc(hWnd, message, wParam, lParam);
	}
}

LRESULT CALLBACK NoPayProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	HDC          hdc;
	PAINTSTRUCT  ps;
	HWND hDialog;
	switch (message)
	{
	case WM_CREATE:
		LVITEM lvItem;
		lvItem.mask = LVIF_TEXT;
		char str[100];
		lvItem.iItem = 0;				// 行
		lvItem.iSubItem = 0;			// 列1
		lvItem.pszText = "abc";			// 内容
		ListView_InsertItem(hWnd, (LPARAM)&lvItem);
		// 列2
		lvItem.iSubItem = 1;
		sprintf(str, "%d", 123);
		lvItem.pszText = str;
		ListView_SetItem(hWnd, (LPARAM)&lvItem);

	case WM_PAINT:
		hdc = BeginPaint(hWnd, &ps);
		EndPaint(hWnd, &ps);
		return 0;
	default:
		return DefWindowProc(hWnd, message, wParam, lParam);
	}
}


LRESULT CALLBACK PayedProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	HDC          hdc;
	PAINTSTRUCT  ps;
	switch (message)
	{
	case WM_PAINT:
	{
		hdc = BeginPaint(hWnd, &ps);
		EndPaint(hWnd, &ps);
	}
	return 0;
	default:
		return DefWindowProc(hWnd, message, wParam, lParam);
	}
}