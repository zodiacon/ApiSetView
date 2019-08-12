#include "pch.h"
#include "ApiSets.h"
#include <wil\resource.h>

#define API_SET_SCHEMA_ENTRY_FLAGS_SEALED 1

typedef struct _API_SET_NAMESPACE {
	ULONG Version;
	ULONG Size;
	ULONG Flags;
	ULONG Count;
	ULONG EntryOffset;
	ULONG HashOffset;
	ULONG HashFactor;
} API_SET_NAMESPACE, * PAPI_SET_NAMESPACE;

typedef struct _API_SET_HASH_ENTRY {
	ULONG Hash;
	ULONG Index;
} API_SET_HASH_ENTRY, * PAPI_SET_HASH_ENTRY;

typedef struct _API_SET_NAMESPACE_ENTRY {
	ULONG Flags;
	ULONG NameOffset;
	ULONG NameLength;
	ULONG HashedLength;
	ULONG ValueOffset;
	ULONG ValueCount;
} API_SET_NAMESPACE_ENTRY, * PAPI_SET_NAMESPACE_ENTRY;

typedef struct _API_SET_VALUE_ENTRY {
	ULONG Flags;
	ULONG NameOffset;
	ULONG NameLength;
	ULONG ValueOffset;
	ULONG ValueLength;
} API_SET_VALUE_ENTRY, * PAPI_SET_VALUE_ENTRY;

ApiSets::ApiSets() {
	Build();
}

const std::vector<ApiSetEntry>& ApiSets::GetApiSets() const {
	return _entries;
}

void ApiSets::Build() {
	auto peb = NtCurrentTeb()->ProcessEnvironmentBlock;
	auto apiSetMap = static_cast<PAPI_SET_NAMESPACE>(peb->Reserved9[0]);
	auto apiSetMapAsNumber = reinterpret_cast<ULONG_PTR>(apiSetMap);

	auto nsEntry = reinterpret_cast<PAPI_SET_NAMESPACE_ENTRY>((apiSetMap->EntryOffset + apiSetMapAsNumber));

	_entries.reserve(apiSetMap->Count);

	for (ULONG i = 0; i < apiSetMap->Count; i++) {
		ApiSetEntry entry;
		entry.Name = CString(reinterpret_cast<PWCHAR>(apiSetMapAsNumber + nsEntry->NameOffset), static_cast<int>(nsEntry->NameLength / sizeof(WCHAR)));
		entry.Sealed = (nsEntry->Flags & API_SET_SCHEMA_ENTRY_FLAGS_SEALED) != 0;
	
		auto valueEntry = reinterpret_cast<PAPI_SET_VALUE_ENTRY>(apiSetMapAsNumber + nsEntry->ValueOffset);
		for (ULONG j = 0; j < nsEntry->ValueCount; j++) {
			CString value(reinterpret_cast<PWCHAR>(apiSetMapAsNumber + valueEntry->ValueOffset), valueEntry->ValueLength / sizeof(WCHAR));
			entry.Values.push_back(value);

			if (valueEntry->NameLength != 0) {
				CString alias(reinterpret_cast<PWCHAR>(apiSetMapAsNumber + valueEntry->NameOffset), valueEntry->NameLength / sizeof(WCHAR));
				entry.Aliases.push_back(alias);
			}

			valueEntry++;
		}
		nsEntry++;
		_entries.push_back(entry);
	}
}

void ApiSets::SearchFiles() {
	WCHAR path[MAX_PATH];
	//::GetWindowsDirectory(path, MAX_PATH);
	//::wcscat_s(path, L"\\Winsxs");
	//SearchDirectory(path, L"api-ms-win-", _files);

	::GetSystemDirectory(path, MAX_PATH);
	::wcscat_s(path, L"\\downlevel");
	SearchDirectory(path, L"api-ms-win-", _files);
}

void ApiSets::SearchDirectory(const CString& directory, const CString& pattern, std::set<CString>& files) {
	WIN32_FIND_DATA fd;
	wil::unique_hfind hFind(::FindFirstFileEx(directory + L"\\*", FindExInfoBasic, &fd, FindExSearchNameMatch, nullptr, 0));
	if (!hFind)
		return;

	do {
		if (wcscmp(fd.cFileName, L".") == 0 || wcscmp(fd.cFileName, L"..") == 0)
			continue;

		if (fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
			SearchDirectory(directory + L"\\" + fd.cFileName, pattern, files);
			continue;
		}

		if(::_wcsnicmp(fd.cFileName, pattern, pattern.GetLength()) == 0)
			files.insert(fd.cFileName);
	} while (::FindNextFile(hFind.get(), &fd));
}
