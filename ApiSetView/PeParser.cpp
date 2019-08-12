#include "pch.h"
#include "PeParser.h"

#include "pch.h"
#include "PEParser.h"
#include <DbgHelp.h>

#pragma comment(lib, "dbghelp")

const int PageSize = 1 << 12;

using namespace std;

void PEParser::CalcHeaders() {
	if (_address == nullptr) {
		_isValid = false;
		return;
	}

	__try {
		_dosHeader = static_cast<IMAGE_DOS_HEADER*>(_address);
		if (_dosHeader->e_lfanew > PageSize || _dosHeader->e_lfanew < 0) {
			_isValid = false;
			return;
		}
		_ntHeaders32 = (IMAGE_NT_HEADERS32*)((BYTE*)_address + _dosHeader->e_lfanew);
		_ntHeaders64 = (IMAGE_NT_HEADERS64*)((BYTE*)_address + _dosHeader->e_lfanew);

		if (_ntHeaders32->OptionalHeader.Magic != IMAGE_NT_OPTIONAL_HDR32_MAGIC &&
			_ntHeaders32->OptionalHeader.Magic != IMAGE_NT_OPTIONAL_HDR64_MAGIC) {
			_isValid = false;
			return;
		}

		_isPE64 = _ntHeaders32->OptionalHeader.Magic == IMAGE_NT_OPTIONAL_HDR64_MAGIC;

		_fileHeader = _isPE64 ? &_ntHeaders64->FileHeader : &_ntHeaders32->FileHeader;

		_header32 = &_ntHeaders32->OptionalHeader;
		_header64 = &_ntHeaders64->OptionalHeader;

		auto offset = _isPE64 ? sizeof(IMAGE_NT_HEADERS64) : sizeof(IMAGE_NT_HEADERS32);
		_sections = reinterpret_cast<IMAGE_SECTION_HEADER*>((BYTE*)_ntHeaders32 + offset);
	}
	__except (EXCEPTION_EXECUTE_HANDLER) {
		// attempt to catch access violations
		_isValid = false;
	}
}

void* PEParser::RvaToAddress(uint32_t rva) const {
	for (int i = 0; i < _fileHeader->NumberOfSections; ++i) {
		const auto& section = _sections[i];
		if (rva >= section.VirtualAddress && rva < section.VirtualAddress + section.Misc.VirtualSize)
			return (BYTE*)_address + section.PointerToRawData + rva - section.VirtualAddress;
	}

	return rva + (BYTE*)_address;
}

std::vector<ExportedSymbol> PEParser::GetExports() const {
	std::vector<ExportedSymbol> exports;
	auto& dir = _isPE64 ? _header64->DataDirectory[IMAGE_DIRECTORY_ENTRY_EXPORT] : _header32->DataDirectory[IMAGE_DIRECTORY_ENTRY_EXPORT];
	if (dir.Size == 0)
		return exports;

	auto exportTable = reinterpret_cast<IMAGE_EXPORT_DIRECTORY*>((BYTE*)_address + dir.VirtualAddress);
	auto nameAddress = reinterpret_cast<char*>((BYTE*)_address + exportTable->Name);
	auto addressOfNames = (BYTE*)_address + exportTable->AddressOfNames;
	auto addressOfOrdinals = (BYTE*)_address + exportTable->AddressOfNameOrdinals;
	auto ordinalBase = exportTable->Base;
	char undecoratedName[256];

	for (DWORD i = 0; i < exportTable->NumberOfNames; i++) {
		uint32_t offset = *(uint32_t*)(addressOfNames + i * 4);
		auto addr = (BYTE*)_address + offset;
		auto name = (const char*)addr;

		ExportedSymbol symbol;
		symbol.Name = name;
		offset = exportTable->AddressOfNameOrdinals + i * 2;
		symbol.Ordinal = *(uint16_t*)((BYTE*)_address + offset) + ordinalBase;

		if (::UnDecorateSymbolName(name, undecoratedName, sizeof(undecoratedName), 0))
			symbol.UndecoratedName = undecoratedName;

		exports.push_back(symbol);
	}

	return exports;
}

PEParser::PEParser(const wchar_t* filename) : PEParser(static_cast<void*>(
	::LoadLibraryEx(filename, nullptr, DONT_RESOLVE_DLL_REFERENCES))) {
}

PEParser::~PEParser() {
	if (_address)
		::FreeLibrary((HMODULE)_address);
}

bool PEParser::IsValidPE() const {
	if (!_isValid)
		return false;

	// look for 'MZ' magic number

	if (_dosHeader->e_magic != IMAGE_DOS_SIGNATURE)
		return false;

	// locate real headers

	if (_ntHeaders32->Signature != 'EP')
		return false;

	if (_fileHeader->SizeOfOptionalHeader < sizeof(IMAGE_OPTIONAL_HEADER32) || _fileHeader->SizeOfOptionalHeader > PageSize)
		return false;

	return true;
}

const char* PEParser::GetExportTableName() const {
	__try {
		auto& dir = _isPE64 ? _header64->DataDirectory[IMAGE_DIRECTORY_ENTRY_EXPORT] : _header32->DataDirectory[IMAGE_DIRECTORY_ENTRY_EXPORT];
		auto exportTable = reinterpret_cast<IMAGE_EXPORT_DIRECTORY*>(RvaToAddress(dir.VirtualAddress));

		// hack!!!
		bool hack = false;
		if (hack || exportTable->Characteristics != 0) {
			hack = true;
			exportTable = reinterpret_cast<IMAGE_EXPORT_DIRECTORY*>((BYTE*)_address + dir.VirtualAddress);
		}

		if (exportTable->Name == 0 || exportTable->Name == 0xffff)
			return "";

		const char* nameAddress;
		if (hack) {
			nameAddress = reinterpret_cast<char*>((BYTE*)_address + exportTable->Name);
		}
		else {
			nameAddress = static_cast<char*>(RvaToAddress(exportTable->Name));
		}

		return nameAddress;
	}
	__except (EXCEPTION_EXECUTE_HANDLER) {
		return "";
	}
}
