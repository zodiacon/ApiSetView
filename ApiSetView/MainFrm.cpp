// MainFrm.cpp : implmentation of the CMainFrame class
//
/////////////////////////////////////////////////////////////////////////////

#include "pch.h"
#include "resource.h"

#include "aboutdlg.h"
#include "MainFrm.h"
#include <functional>
#include <algorithm>

BOOL CMainFrame::PreTranslateMessage(MSG* pMsg) {
	if (CFrameWindowImpl<CMainFrame>::PreTranslateMessage(pMsg))
		return TRUE;

	return FALSE;
}

BOOL CMainFrame::OnIdle() {
	UIUpdateToolBar();
	return FALSE;
}

void CMainFrame::DoSort(const CVirtualListView::SortInfo* si) {
	switch (si->Id) {
		case IDC_APISET:
			std::sort(m_Entries.begin(), m_Entries.end(), [si](auto& e1, auto& e2) { return CompareApiSets(e1, e2, si); });
			break;

		case IDC_EXPORTS:
			std::sort(m_Symbols.begin(), m_Symbols.end(), [si](auto& e1, auto& e2) { return CompareExports(e1, e2, si); });
			break;

		case IDC_APISETEXPORTS:
			std::sort(m_ApiSetExports.begin(), m_ApiSetExports.end(), [si](auto& e1, auto& e2) { return CompareExports(e1, e2, si); });
			break;

		default:
			ATLASSERT(false);
			break;
	}

}

bool CMainFrame::CompareApiSets(const ApiSetEntry& e1, const ApiSetEntry& e2, const CVirtualListView::SortInfo* si) {
	switch (si->SortColumn) {
		case 0:		// name
			return SortStrings(e1.Name, e2.Name, si->SortAscending);

		case 1:		// host
			return SortStrings(
				e1.Values.empty() ? L"" : e1.Values[0], 
				e2.Values.empty() ? L"" : e2.Values[0], 
				si->SortAscending);

	}
	ATLASSERT(false);
	return false;
}

bool CMainFrame::CompareExports(const ExportedSymbol& e1, const ExportedSymbol& e2, const CVirtualListView::SortInfo* si) {
	switch (si->SortColumn) {
		case 0:		// name
			return SortStrings(e1.Name, e2.Name, si->SortAscending);

		case 1:		// ordinal
			return SortNumbers(e1.Ordinal, e2.Ordinal, si->SortAscending);

		case 2:		// undecorated name
			return SortStrings(e1.UndecoratedName, e2.UndecoratedName, si->SortAscending);
	}
	ATLASSERT(false);
	return false;
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

	m_hWndClient = m_Splitter.Create(m_hWnd, rcDefault, nullptr, WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS | WS_CLIPCHILDREN);
	m_Splitter2.Create(m_Splitter, rcDefault, nullptr, WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS | WS_CLIPCHILDREN | WS_BORDER);

	m_ExportListPane.SetPaneContainerExtendedStyle(PANECNT_NOCLOSEBUTTON);
	m_ExportListPane.Create(m_Splitter2, _T("Host Exports"), WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS | WS_CLIPCHILDREN);

	m_ApiSetExportPane.SetPaneContainerExtendedStyle(PANECNT_NOCLOSEBUTTON);
	m_ApiSetExportPane.Create(m_Splitter2, _T("API Set Exports"), WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS | WS_CLIPCHILDREN);

	m_ApiSetList.Create(m_Splitter, rcDefault, nullptr, WS_CHILD | WS_VISIBLE | LVS_SINGLESEL | LVS_OWNERDATA |
		LVS_REPORT | LVS_SHOWSELALWAYS | WS_CLIPCHILDREN | WS_CLIPSIBLINGS, 0, IDC_APISET);

	m_ExportList.Create(m_ExportListPane, rcDefault, nullptr, WS_CHILD | WS_VISIBLE | LVS_SINGLESEL | LVS_OWNERDATA |
		LVS_REPORT | LVS_SHOWSELALWAYS | WS_CLIPCHILDREN | WS_CLIPSIBLINGS, 0, IDC_EXPORTS);
	m_ApiSetList.SetExtendedListViewStyle(LVS_EX_DOUBLEBUFFER | LVS_EX_FULLROWSELECT, 0);

	m_ApiSetExportList.Create(m_ApiSetExportPane, rcDefault, nullptr, WS_CHILD | WS_VISIBLE | LVS_SINGLESEL | LVS_OWNERDATA |
		LVS_REPORT | LVS_SHOWSELALWAYS | WS_CLIPCHILDREN | WS_CLIPSIBLINGS, 0, IDC_APISETEXPORTS);
	m_ApiSetExportList.SetExtendedListViewStyle(LVS_EX_DOUBLEBUFFER | LVS_EX_FULLROWSELECT, 0);
	m_ApiSetExportPane.SetClient(m_ApiSetExportList);

	m_ExportList.SetExtendedListViewStyle(LVS_EX_DOUBLEBUFFER | LVS_EX_FULLROWSELECT, 0);
	m_ExportListPane.SetClient(m_ExportList);

	m_Splitter2.SetSplitterPanes(m_ApiSetExportPane, m_ExportListPane);
	m_Splitter2.SetSplitterPosPct(30);

	m_Splitter.SetSplitterPanes(m_ApiSetList, m_Splitter2);
	m_Splitter.SetSplitterPosPct(35);

	m_ApiSetList.InsertColumn(0, L"API Set Name", LVCFMT_LEFT, 300);
	m_ApiSetList.InsertColumn(1, L"Host(s)", LVCFMT_LEFT, 200);
	//m_ApiSetList.InsertColumn(2, L"Aliases(s)", LVCFMT_LEFT, 120);
	//m_ApiSetList.InsertColumn(3, L"Sealed?", LVCFMT_LEFT, 100);

	m_ExportList.InsertColumn(0, L"Name", LVCFMT_LEFT, 300);
	m_ExportList.InsertColumn(1, L"Ordinal", LVCFMT_RIGHT, 80);
	m_ExportList.InsertColumn(2, L"Undecorated Name", LVCFMT_LEFT, 300);

	m_ApiSetExportList.InsertColumn(0, L"Name", LVCFMT_LEFT, 300);
	m_ApiSetExportList.InsertColumn(1, L"Ordinal", LVCFMT_RIGHT, 80);
	m_ApiSetExportList.InsertColumn(2, L"Undecorated Name", LVCFMT_LEFT, 300);

	m_Images.Create(16, 16, ILC_COLOR32 | ILC_COLOR, 2, 2);
	m_Images.AddIcon(AtlLoadIcon(IDI_ARROW));
	m_Images.AddIcon(AtlLoadIcon(IDI_CIRCLE));
	m_ApiSetList.SetImageList(m_Images, LVSIL_SMALL);

	//UIAddToolBar(hWndToolBar);
	UISetCheck(ID_VIEW_TOOLBAR, 1);
	UISetCheck(ID_VIEW_STATUS_BAR, 1);

	CReBarCtrl(m_hWndToolBar).LockBands(TRUE);

	// register object for message filtering and idle updates
	CMessageLoop* pLoop = _Module.GetMessageLoop();
	ATLASSERT(pLoop != nullptr);
	pLoop->AddMessageFilter(this);
	pLoop->AddIdleHandler(this);

	m_Entries = m_ApiSets.GetApiSets();
	m_ApiSetList.SetItemCount(static_cast<int>(m_Entries.size()));
	m_ApiSets.SearchFiles();

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

	switch (hdr->idFrom) {
		case IDC_APISET:
		{
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
			if (lv->item.mask & LVIF_IMAGE) {
				item.iImage = m_ApiSets.IsFileExists(data.Name + L".dll") ? 0 : 1;
			}
			break;
		}

		case IDC_EXPORTS:
		case IDC_APISETEXPORTS:
		{
			// exports
			const auto& data = hdr->idFrom == IDC_EXPORTS ? m_Symbols[index] : m_ApiSetExports[index];
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
			break;
		}
	}

	return 0;
}

LRESULT CMainFrame::OnItemChanged(int, LPNMHDR hdr, BOOL&) {
	if (hdr->idFrom != IDC_APISET)
		return 0;

	static WCHAR ApiSetDir[MAX_PATH];
	if (*ApiSetDir == 0) {
		::GetSystemDirectory(ApiSetDir, MAX_PATH);
		::wcscat_s(ApiSetDir, L"\\downlevel\\");
	}

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
		ClearSort(IDC_EXPORTS);

		m_ApiSetExportList.SetItemCount(0);
		if (m_ApiSets.IsFileExists(item.Name + L".dll")) {
			PEParser parser(ApiSetDir + item.Name + L".dll");
			if (parser.IsValidPE()) {
				m_ApiSetExports = parser.GetExports();
				const auto count = static_cast<int>(m_ApiSetExports.size());
				m_ApiSetExportList.SetItemCount(count);
				m_ApiSetExportList.EnableWindow(TRUE);
				m_ApiSetExportList.RedrawItems(0, count);
				ClearSort(IDC_APISETEXPORTS);
			}
		}
		else {
			m_ApiSetExportList.EnableWindow(FALSE);
		}

		m_ExportList.LockWindowUpdate(FALSE);
	}
	return 0;
}

LRESULT CMainFrame::OnFindItem(int, LPNMHDR hdr, BOOL&) {
	CListViewCtrl* pList;
	std::function<CString(int)> pGetValue;

	switch (hdr->idFrom) {
		case IDC_APISET:
			pList = &m_ApiSetList;
			pGetValue = [this](int i) { return GetItem(i).Name; };
			break;

		case IDC_EXPORTS:
			pList = &m_ExportList; 
			pGetValue = [this](int i) { return CString(m_Symbols[i].Name.c_str()); };
			break;

		case IDC_APISETEXPORTS:
			pList = &m_ApiSetExportList;
			pGetValue = [this](int i) { return CString(m_ApiSetExports[i].Name.c_str()); };
			break;

		default:
			ATLASSERT(false);
			return -1;
	}

	auto fi = (NMLVFINDITEM*)hdr;
	if (fi->lvfi.flags & (LVFI_PARTIAL | LVFI_SUBSTRING)) {
		int start = fi->iStart;
		auto count = pList->GetItemCount();
		auto text = fi->lvfi.psz;
		auto len = ::wcslen(text);
		for (int i = start; i < start + count; i++) {
			auto index = i % count;
			if (::_wcsnicmp(pGetValue(index), text, len) == 0)
				return index;
		}
	}
	return -1;
}
