public Plugin:myinfo = {
    name = "Test Async",
    author = "Bottiger",
    description = "Test the Async Extension",
    version = "1",
    url = "https://www.skial.com"
};

#pragma dynamic 16000

#include <async>
#include <regex>

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

    bool test = GetFeatureStatus(FeatureType_Native, "Async_CurlPostRawCompress") == FeatureStatus_Available; // will only show true if the plugin uses it
    PrintToServer("Async_CurlPostRawCompress available %i", test);
}

public Action:Cmd_Test2(client, args) {
	char url[256];
	GetCmdArg(1, url, sizeof(url));

	CurlHandle r = Async_CurlNew();
	Async_CurlSetInt(r, CURLOPT_VERBOSE, 1);
	//Async_CurlAddHeader(r, "Expect: "); // this is needed to prevent curl from doing nothing on post sizes greater than 1000 bytes???
	Async_CurlSetString(r, CURLOPT_COOKIEJAR, "/tmp/cookies.txt");
	Async_CurlSetString(r, CURLOPT_COOKIE, "");
	Async_CurlGet(r, "", Callback2);
}

public Action:Cmd_Test(client, args) 
{
	char url[256];
	GetCmdArg(1, url, sizeof(url));

	CurlHandle r = Async_CurlNew();
	Async_CurlSetInt(r, CURLOPT_VERBOSE, 1);
	Async_CurlSetString(r, CURLOPT_ACCEPT_ENCODING, "br");
	Async_CurlGet(r, url, Callback);
}

public Callback(CurlHandle:request, curlcode, httpcode, size) 
{
	if(curlcode != 0) {
		char curlcode_string[CURL_ERROR_SIZE];
		char err[CURL_ERROR_SIZE];

		Async_CurlErrorString(curlcode, curlcode_string, sizeof(curlcode_string));
		Async_CurlGetErrorMessage(request, err, sizeof(err));

		PrintToServer("Request failed: curl %i (%s) %s", curlcode, curlcode_string, err);
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

/*
	int headerSize = Async_CurlGetResponseHeaderSize(request, "date");
	char[] header = new char[headerSize+1];
	Async_CurlGetResponseHeader(request, "date", header, headerSize+1);
	PrintToServer("date header: %s", header);
*/

	Async_Close(request);
}

public Callback2(CurlHandle:request, curlcode, httpcode, size) 
{
	if(curlcode != 0) {
		PrintToServer("Request failed: curl %i", curlcode);
		Async_Close(request);
		return;
	}
	if(httpcode != 200) {
		PrintToServer("Request failed: httpcode %i", httpcode);
	}
	if(size > 64000)
	{
		size = 64000;
	}

	char[] data = new char[size+1];
	Async_CurlGetData(request, data, size);
	Async_Close(request);

	Regex pattern = new Regex("\"teamwork.tf:validation:[0-9a-f]+\"");
	Regex token_pattern = new Regex("name=\"_token\" value=\"(.+?)\"");

	char name[64];
	char token[64];

	if(pattern.Match(data))
	{
		pattern.GetSubString(0, name, sizeof(name));
	}
	if(token_pattern.Match(data))
	{
		token_pattern.GetSubString(1, token, sizeof(token));
	}

	PrintToServer("name: %s token: %s", name, token);
	delete pattern;
	delete token_pattern;

	char post[256];
	Format(post, sizeof(post), "ip=%s&port=%i&whitelist_ip=on&_token=%s", "91.216.250.11", 27015, token);

	CurlHandle r = Async_CurlNew();
	Async_CurlSetInt(r, CURLOPT_VERBOSE, 1);
	Async_CurlSetString(r, CURLOPT_COOKIEFILE, "/tmp/cookies.txt");
	Async_CurlSetString(r, CURLOPT_COOKIEJAR, "/tmp/cookies.txt");
	//Async_CurlAddHeader(r, "Origin: https://teamwork.tf");
	Async_CurlPost(r, "https://teamwork.tf/settings/manage/communityserverprovider/skial/validate-server", post, Callback3);
}

public Callback3(CurlHandle:request, curlcode, httpcode, size) 
{
	if(curlcode != 0) {
		PrintToServer("Request failed: curl %i", curlcode);
		Async_Close(request);
		return;
	}
	if(httpcode != 200) {
		PrintToServer("Request failed: httpcode %i", httpcode);
	}

	char[] data = new char[size+1];
	Async_CurlGetData(request, data, size);
	Async_Close(request);

	File f = OpenFile("dump.txt", "w");
	f.WriteString(data, false);
	delete f;
}