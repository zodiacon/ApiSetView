#pragma once

struct ApiSetEntry {
	CString Name;
	std::vector<CString> Values, Aliases;
	bool Sealed;
};

class ApiSets {
public:
	ApiSets();

	void SearchFiles();

	const std::vector<ApiSetEntry>& GetApiSets() const;

private:
	void Build();
	void SearchDirectory(const CString& directory, const CString& pattren, std::set<CString>& files);

private:
	std::vector<ApiSetEntry> _entries;
	std::set<CString> _files;
};

