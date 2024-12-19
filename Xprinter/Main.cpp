#include <iostream>
#include <string>
#include <tchar.h> 
#include <sstream>
#include <fstream>
#include <json.hpp> 
//#include <Windows.h>
#include "./httplib.h"

using json = nlohmann::json;
using namespace std;

typedef void* (__cdecl* pInitPrinter)(const TCHAR* model);
typedef int(__cdecl* pOpenPort)(void* handle, const TCHAR* settin);
typedef int(__cdecl* pWriteData)(void* handle, unsigned char* buffer, size_t size);
typedef int(__cdecl* pPrinterInitialize)(void* handle);
typedef int(__cdecl* pPrintAndFeedLine)(void* hPrinter);
typedef int(__cdecl* pCutPaperWithDistance)(void* hPrinter, int distance);
typedef int(__cdecl* pGetPrinterState)(void* hPrinter, unsigned int* printerStatus);
typedef int(__cdecl* pPrintSymbol)(void* hPrinter, int type, const char* data, int errLevel, int width, int height, int alignment);
typedef int(__cdecl* pCutPaper)(void* hPrinter, int cutMode);

string strCenter = "\x1B\x61\x31"; // 중앙정렬
string strLeft = "\x1B\x61\x30"; // 왼쪽정렬
string strRight = "\x1B\x61\x32"; // 오른쪽정렬
string strDouble = "\x1B\x21\x20"; // Horizontal Double
string strUnderline = "\x1B\x21\x80"; // underline
string strDoubleBold = "\x1B\x21\x28"; // Emphasize
string strNormal = "\x1B\x21\x02"; // 중앙정렬

int main() {
	std::ifstream configFile("config.ini");
	if (!configFile.is_open()) {
		printf("\nconfig.ini file not found.");
		Sleep(3000);
		main();
	}
	std::string lineConfig;
	int port = 0;
	while (std::getline(configFile, lineConfig)) {
		if (lineConfig.find("PORT=") == 0) {
			std::string portStr = lineConfig.substr(5);
			std::stringstream(portStr) >> port;
			break;
		}
	}
	configFile.close();
	if (port == 0) {
		printf("\nPort not found in config.ini file");
		Sleep(3000);
		main();
	}
	printf("\nPort : %d\n", port);

	// Upload DLL file
	const wchar_t* dllPath = L"printer.sdk.dll";
	HMODULE hDllInst = LoadLibrary(dllPath);
	if (!hDllInst) {
		printf("\nprinter.sdk.dll file not found");
		Sleep(3000);
		main();
	}
	pInitPrinter InitPrinter = (pInitPrinter)GetProcAddress(hDllInst, "InitPrinter");
	pOpenPort OpenPort = (pOpenPort)GetProcAddress(hDllInst, "OpenPort");
	pPrinterInitialize PrinterInitialize = (pPrinterInitialize)GetProcAddress(hDllInst, "PrinterInitialize");
	pPrintAndFeedLine PrintAndFeedLine = (pPrintAndFeedLine)GetProcAddress(hDllInst, "PrintAndFeedLine");
	pCutPaperWithDistance CutPaperWithDistance = (pCutPaperWithDistance)GetProcAddress(hDllInst, "CutPaperWithDistance");
	pWriteData WriteData = (pWriteData)GetProcAddress(hDllInst, "WriteData");
	pGetPrinterState GetPrinterState = (pGetPrinterState)GetProcAddress(hDllInst, "GetPrinterState");
	pPrintSymbol PrintSymbol = (pPrintSymbol)GetProcAddress(hDllInst, "PrintSymbol");
	pCutPaper CutPaper = (pCutPaper)GetProcAddress(hDllInst, "CutPaper");

	if (!InitPrinter || !OpenPort ||
		!PrinterInitialize || !PrintAndFeedLine ||
		!CutPaperWithDistance || !WriteData ||
		!GetPrinterState || !PrintSymbol || !CutPaper) {
		printf("\nFunctions not found in DLL file");
		FreeLibrary(hDllInst);
		Sleep(3000);
		main();
	}

	const TCHAR* printerModel = _T("XP-T80Q");
	void* printerHandle = InitPrinter(printerModel);
	int result = OpenPort(printerHandle, L"USB,");
	if (result != 0) {
		printf("\nError on open port");
		Sleep(3000);
		main();
	}
	else {
		printf("Port opened\n");
	}


    httplib::Server svr;
    svr.Post(R"(/.*)", [&CutPaperWithDistance, &printerHandle, &GetPrinterState, &PrintSymbol, &result, &PrintAndFeedLine, &WriteData, &PrinterInitialize, &OpenPort](const httplib::Request& req, httplib::Response& res) {
        try {
            auto jsonArray = json::parse(req.body);
            if (jsonArray.is_array()) {
				for (const auto& element : jsonArray) {
					if ((!element.contains("type") || !element.contains("align") || !element.contains("font") || !element.contains("body")) ||
						(element["type"] != "text" && element["type"] != "qrCode") ||
						(element["align"] != "center" && element["align"] != "left" && element["align"] != "right") ||
						(element["font"] != "normal" && element["font"] != "bold" && element["font"] != "large" && element["font"] != "underline")) {
						res.status = 400;
						res.set_content("JSON elements are invalid", "text/plain");
						return;
					}
				}
				PrinterInitialize(printerHandle);
				unsigned int statusPrinter = 2;
				int hStatusPrinter = GetPrinterState(printerHandle, &statusPrinter);
				if (hStatusPrinter == 0) {
					if (0x12 == statusPrinter) {
						for (const auto& element : jsonArray) {
							string printString = "";
							if (element["type"] == "text") {
								// Find align 
								if (element["align"] == "center") { printString = printString + strCenter; }
								else if (element["align"] == "right") { printString = printString + strRight; }
								else { printString = printString + strLeft; }
								// Find font
								if (element["font"] == "bold") { printString = printString + strDoubleBold; }
								else if (element["font"] == "large") { printString = printString + strDouble; }
								else if (element["font"] == "underline") { printString = printString + strUnderline; }
								else { printString = printString + strNormal; }
								// Printing process
								printString = printString + element["body"].get<std::string>() + "\r\n";
								unsigned char* buffer = new unsigned char[printString.length() + 1];
								for (size_t i = 0; i < printString.length(); ++i) {
									buffer[i] = static_cast<unsigned char>(printString[i]);
								}
								buffer[printString.length()] = '\0';  
								WriteData(printerHandle, buffer, printString.size());
								delete[] buffer;
							}
							else if (element["type"] == "qrCode") {
								const std::string& body = element["body"].get<std::string>();
								char* strQRCode = new char[body.size() + 1];
								strcpy_s(strQRCode, body.size() + 1, body.c_str());
								if (element["align"] == "right") {
									PrintSymbol(printerHandle, 49, strQRCode, 49, 9, 9, 2);
								}
								else if (element["align"] == "center") {
									PrintSymbol(printerHandle, 49, strQRCode, 49, 9, 9, 1);
								}
								else {
									PrintSymbol(printerHandle, 49, strQRCode, 49, 9, 9, 0);
								}
								delete[] strQRCode;
							}
						}
					}
					else
					{
						if (0x12 == statusPrinter)
						{
							res.status = 400;
							res.set_content("Ready", "text/plain");
						}
						else if ((statusPrinter & 0b100) > 0)
						{
							res.status = 400;
							res.set_content("Cover opened", "text/plain");
						}
						else if ((statusPrinter & 0b1000) > 0)
						{
							res.status = 400;
							res.set_content("Feed button has been pressed", "text/plain");
						}
						else if ((statusPrinter & 0b100000) > 0)
						{
							res.status = 400;
							res.set_content("Printer is out of paper", "text/plain");
						}
						else if ((statusPrinter & 0b1000000) > 0)
						{
							res.status = 400;
							res.set_content("Error condition", "text/plain");
						}
						else
						{
							res.status = 400;
							res.set_content("Error", "text/plain");
						}
						return;
					}
				}
				else {
					res.status = 400;
					res.set_content("Power off", "text/plain");
					result = OpenPort(printerHandle, L"USB,");
					return;
				}
				PrintAndFeedLine(printerHandle);
				PrintAndFeedLine(printerHandle);
                CutPaperWithDistance(printerHandle, 255);
            }
            else {
                res.status = 400;
                res.set_content("Array elements are invalid", "text/plain");
                return;
            }
        }
        catch (const json::parse_error& e) {
            res.status = 400;
            res.set_content("The data sent is not in json format", "text/plain");
            return;
        }
        });
    svr.listen("localhost", port);
    main();
}