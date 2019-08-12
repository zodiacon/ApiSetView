// MainFrm.cpp : implmentation of the CMainFrame class
//
/////////////////////////////////////////////////////////////////////////////

#include "pch.h"
#include "resource.h"

#include "aboutdlg.h"
#include "MainFrm.h"

BOOL CMainFrame::PreTranslateMessage(MSG* pMsg) {
	if (CFrameWindowImpl<CMainFrame>::PreTranslateMessage(pMsg))
		return TRUE;

	return FALSE;
}

BOOL CMainFrame::OnIdle() {
	UIUpdateToolBar();
	return FALSE;
}

LRESULT CMainFrame::OnCreate(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/) {
	// create command bar window
	HWND hWndCmdBar = m_CmdBar.Create(m_hWnd, rcDefault, nullptr, ATL_SIMPLE_CMDBAR_PANE_STYLE);
	// attach menu
	m_CmdBar.AttachMenu(GetMenu());
	// load command bar images
	m_CmdBar.LoadImages(IDR_MAINFRAME);
	// remove old menu
	SetMenu(nullptr);

	//	HWND hWndToolBar = CreateSimpleToolBarCtrl(m_hWnd, IDR_MAINFRAME, FALSE, ATL_SIMPLE_TOOLBAR_PANE_STYLE);

	CreateSimpleReBar(ATL_SIMPLE_REBAR_NOBORDER_STYLE);
	AddSimpleReBarBand(hWndCmdBar);
	//AddSimpleReBarBand(hWndToolBar, nullptr, TRUE);

	CreateSimpleStatusBar();

	m_hWndClient = m_splitter.Create(m_hWnd, rcDefault, nullptr, WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS | WS_CLIPCHILDREN);
	m_ApiSetList.Create(m_splitter, rcDefault, nullptr, WS_CHILD | WS_VISIBLE | LVS_SINGLESEL | LVS_OWNERDATA |
		LVS_REPORT | LVS_SHOWSELALWAYS | WS_CLIPCHILDREN | WS_CLIPSIBLINGS, 0, IDC_APISET);
	m_ExportList.Create(m_splitter, rcDefault, nullptr, WS_CHILD | WS_VISIBLE | LVS_SINGLESEL | LVS_OWNERDATA |
		LVS_REPORT | LVS_SHOWSELALWAYS | WS_CLIPCHILDREN | WS_CLIPSIBLINGS, 0, IDC_EXPORTS);
	m_ApiSetList.SetExtendedListViewStyle(LVS_EX_DOUBLEBUFFER | LVS_EX_FULLROWSELECT, 0);
	m_ExportList.SetExtendedListViewStyle(LVS_EX_DOUBLEBUFFER | LVS_EX_FULLROWSELECT, 0);

	m_splitter.SetSplitterPanes(m_ApiSetList, m_ExportList);
	m_splitter.SetSplitterPosPct(40);

	m_ApiSetList.InsertColumn(0, L"API Set Name", LVCFMT_LEFT, 320);
	m_ApiSetList.InsertColumn(1, L"Host(s)", LVCFMT_LEFT, 200);
	m_ApiSetList.InsertColumn(2, L"Aliases(s)", LVCFMT_LEFT, 150);
	//m_ApiSetList.InsertColumn(3, L"Sealed?", LVCFMT_LEFT, 100);

	m_ExportList.InsertColumn(0, L"Export Name", LVCFMT_LEFT, 350);
	m_ExportList.InsertColumn(1, L"Ordinal", LVCFMT_RIGHT, 80);
	m_ExportList.InsertColumn(2, L"Undecorated Name", LVCFMT_LEFT, 350);

	//UIAddToolBar(hWndToolBar);
	UISetCheck(ID_VIEW_TOOLBAR, 1);
	UISetCheck(ID_VIEW_STATUS_BAR, 1);

	CReBarCtrl(m_hWndToolBar).LockBands(TRUE);

	// register object for message filtering and idle updates
	CMessageLoop* pLoop = _Module.GetMessageLoop();
	ATLASSERT(pLoop != nullptr);
	pLoop->AddMessageFilter(this);
	pLoop->AddIdleHandler(this);

	ApiSets sets;
	m_Entries = sets.GetApiSets();
	m_ApiSetList.SetItemCount(static_cast<int>(m_Entries.size()));
	sets.SearchFiles();

	return 0;
}

LRESULT CMainFrame::OnDestroy(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& bHandled) {
	// unregister message filtering and idle updates
	CMessageLoop* pLoop = _Module.GetMessageLoop();
	ATLASSERT(pLoop != nullptr);
	pLoop->RemoveMessageFilter(this);
	pLoop->RemoveIdleHandler(this);

	bHandled = FALSE;
	return 1;
}

LRESULT CMainFrame::OnFileExit(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
	PostMessage(WM_CLOSE);
	return 0;
}

LRESULT CMainFrame::OnViewToolBar(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
	static BOOL bVisible = TRUE;	// initially visible
	bVisible = !bVisible;
	CReBarCtrl rebar = m_hWndToolBar;
	int nBandIndex = rebar.IdToIndex(ATL_IDW_BAND_FIRST + 1);	// toolbar is 2nd added band
	rebar.ShowBand(nBandIndex, bVisible);
	UISetCheck(ID_VIEW_TOOLBAR, bVisible);
	UpdateLayout();
	return 0;
}

LRESULT CMainFrame::OnViewStatusBar(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
	BOOL bVisible = !::IsWindowVisible(m_hWndStatusBar);
	::ShowWindow(m_hWndStatusBar, bVisible ? SW_SHOWNOACTIVATE : SW_HIDE);
	UISetCheck(ID_VIEW_STATUS_BAR, bVisible);
	UpdateLayout();
	return 0;
}

LRESULT CMainFrame::OnAppAbout(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
	CAboutDlg dlg;
	dlg.DoModal();
	return 0;
}

const ApiSetEntry& CMainFrame::GetItem(int index) const {
	return m_Entries[index];
}

CString CMainFrame::StringVectorToString(const std::vector<CString>& names) const {
	CString text;
	for (auto& value : names)
		text += value + L", ";
	return text.Left(text.GetLength() - 2);
}

LRESULT CMainFrame::OnGetDispInfo(int, LPNMHDR hdr, BOOL&) {
	auto lv = reinterpret_cast<NMLVDISPINFO*>(hdr);
	auto& item = lv->item;
	auto index = item.iItem;
	auto col = item.iSubItem;

	if (hdr->idFrom == IDC_APISET) {
		auto& data = GetItem(index);

		if (lv->item.mask & LVIF_TEXT) {
			switch (col) {
				case 0:		// name
					item.pszText = (PWSTR)(PCWSTR)data.Name;
					break;

				case 1:		// hosts
					::StringCchCopy(item.pszText, item.cchTextMax, StringVectorToString(data.Values));
					break;

				case 2:		// aliases
					::StringCchCopy(item.pszText, item.cchTextMax, StringVectorToString(data.Aliases));
					break;

				case 3:		// sealed?
					::StringCchCopy(item.pszText, item.cchTextMax, data.Sealed ? L"Yes" : L"No");
					break;
			}
		}
	}
	else {
		// exports
		const auto& data = m_Symbols[index];
		switch (col) {
			case 0:		// name
				::StringCchCopy(item.pszText, item.cchTextMax, CString(data.Name.c_str()));
				break;

			case 1:		// ordinal
				::StringCchPrintf(item.pszText, item.cchTextMax, L"%d", data.Ordinal);
				break;

			case 2:		// undecorated name
				::StringCchCopy(item.pszText, item.cchTextMax, CString(data.UndecoratedName.c_str()));
				break;
		}
	}

	return 0;
}

LRESULT CMainFrame::OnItemChanged(int, LPNMHDR hdr, BOOL&) {
	if (hdr->idFrom != IDC_APISET)
		return 0;

	static int oldSelected = -1;
	auto selected = m_ApiSetList.GetSelectedIndex();
	if (selected < 0 || selected == oldSelected)
		return 0;

	oldSelected = selected;
	const auto& item = GetItem(selected);
	PEParser parser(item.Name);
	if (!parser.IsValidPE()) {
		m_ExportList.SetItemCount(0);
	}
	else {
		m_ExportList.LockWindowUpdate();
		m_Symbols = parser.GetExports();
		m_ExportList.SetItemCount(static_cast<int>(m_Symbols.size()));
		m_ExportList.RedrawItems(0, m_ExportList.GetCountPerPage());
		m_ExportList.LockWindowUpdate(FALSE);
	}
	return 0;
}
