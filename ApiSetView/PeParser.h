#pragma once

struct ExportedSymbol final {
	std::string Name;
	uint32_t Ordinal;
	uint32_t Address;
	std::string ForwardName, UndecoratedName;
};

class PEParser final {
public:
	explicit PEParser(void* address) : _address(address) { CalcHeaders(); }
	explicit PEParser(const wchar_t* filename);
	~PEParser();

	bool IsValidPE() const;
	const char* GetExportTableName() const;

	void* RvaToAddress(uint32_t rva) const;
	std::vector<ExportedSymbol> GetExports() const;

private:
	void CalcHeaders();

private:
	struct SectionInfo {
		uint32_t Address, Size;
	};

	IMAGE_OPTIONAL_HEADER64* _header64;
	IMAGE_OPTIONAL_HEADER32* _header32;
	IMAGE_DOS_HEADER* _dosHeader;
	IMAGE_FILE_HEADER* _fileHeader;
	IMAGE_NT_HEADERS32* _ntHeaders32;
	IMAGE_NT_HEADERS64* _ntHeaders64;
	IMAGE_SECTION_HEADER* _sections;
	void* _address;
	HANDLE _hMapFile{ nullptr };
	bool _isValid = true;
	bool _isPE64;
};

