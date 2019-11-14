public Plugin:myinfo = {
    name = "Test Async",
    author = "Bottiger",
    description = "Test the Async Extension",
    version = "1",
    url = "https://www.skial.com"
};

#include <async>

public APLRes AskPluginLoad2(Handle myself, bool late, char[] error, int err_max)
{
	MarkNativeAsOptional("Async_CurlPostRawCompress");
	return APLRes_Success;
}

public OnPluginStart() {
    RegConsoleCmd("sm_test", Cmd_Test);
    RegConsoleCmd("sm_test2", Cmd_Test2);

    char version[256];
    Async_CurlVersion(version, sizeof(version));
    PrintToServer(version);

    bool test = GetFeatureStatus(FeatureType_Native, "Async_CurlPostRawCompress") == FeatureStatus_Available;
    PrintToServer("Async_CurlPostRawCompress available %i", test);
}

public Action:Cmd_Test2(client, args) {
	char url[256];
	GetCmdArg(1, url, sizeof(url));

	char[] data = "abcdefghijklmnop";
	new CurlHandle:r = Async_CurlNew();
	Async_CurlSetInt(r, CURLOPT_VERBOSE, 1);
	Async_CurlAddHeader(r, "Expect: "); // this is needed to prevent curl from doing nothing on post sizes greater than 1000 bytes???
	Async_CurlPostRawCompress(r, url, data, strlen(data), Callback);
}

public Action:Cmd_Test(client, args) {
	char url[256];
	GetCmdArg(1, url, sizeof(url));

	CurlHandle r = Async_CurlNew();
	Async_CurlSetInt(r, CURLOPT_VERBOSE, 1);
	Async_CurlSetString(r, CURLOPT_ACCEPT_ENCODING, "br");
	Async_CurlGet(r, url, Callback);
}

public Callback(CurlHandle:request, curlcode, httpcode, size) {
	if(curlcode != 0) {
		PrintToServer("Request failed: curl %i", curlcode);
		Async_Close(request);
		return;
	}
	if(httpcode != 200) {
		PrintToServer("Request failed: httpcode %i", httpcode);
	}

	PrintToServer("response size: %i WARNING LARGE RESPONSE SIZES WILL CAUSE Sourcemod to silently stop executing code! Might be fixable with #pragma dynamic 32768", size);
	char[] data = new char[size+1];

	Async_CurlGetData(request, data, size);
	PrintToServer("result %s", data);

	int headerSize = Async_CurlGetResponseHeaderSize(request, "date");
	char[] header = new char[headerSize+1];
	Async_CurlGetResponseHeader(request, "date", header, headerSize+1);
	PrintToServer("date header: %s", header);

	Async_Close(request);
}