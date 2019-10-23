#include "main.h"

void setup()
{
	Serial.begin(115200);
	pinMode(LED_BUILTIN, OUTPUT);

	digitalWrite(LED_BUILTIN, HIGH);

	pinMode(_pinHeat, OUTPUT);							// Output mode to drive relay
	digitalWrite(_pinHeat, _invertRelay ? HIGH : LOW);  // Make sure it is off to start
	pinMode(_pinLight, OUTPUT);							// Output mode to drive relay
	digitalWrite(_pinLight, _invertRelay ? HIGH : LOW); // make sure it is off to start

	//delay(10 * 1000);

	SetupBmp280();
	SetupTime();

	digitalWrite(LED_BUILTIN, LOW);
}

void loop()
{
	unsigned long currentMillis = millis();

	HandleTemps(currentMillis);
	HandleLights(currentMillis);

	//showTime();
	delay(1000);
}

void SetupBmp280()
{

	if (!_bmp.begin())
	{
		Serial.println("Could not find a valid BMP280 sensor, check wiring!");
		while (1)
		{
			digitalWrite(LED_BUILTIN, LOW);
			delay(200);
			digitalWrite(LED_BUILTIN, HIGH);
			delay(200);
		}
	}
}

void SetupTime()
{
	digitalWrite(LED_BUILTIN, HIGH);

	while (!HasTime())
	{
		Serial.println("Time...");
		UpdateTime();
		if (!HasTime())
		{
			Serial.println("Time :-(");
			for (int i = 60; i > 0; i--)
			{
				Serial.printf("Retrying in %i seconds...\r", i);
				delay(1000);
			}
			Serial.println("");
		}
	}
	digitalWrite(LED_BUILTIN, LOW);
}

void HandleLights(unsigned long currentMillis)
{
	if (currentMillis - _lastLightTime > _lightInterval)
	{
		// TODO: Make this a little more robust ;-)
		time_t utc = time(nullptr);
		struct tm timeinfo;
		gmtime_r(&utc, &timeinfo);

		if (timeinfo.tm_mon < 4 || timeinfo.tm_mon >= 8) // only use lights in Sept - April
		{
			if ((timeinfo.tm_hour >= 12 && timeinfo.tm_hour <= 13) || timeinfo.tm_hour >= 21 || timeinfo.tm_hour <= 0) // 8 - 10am and 5 - 9pm
			{
				digitalWrite(_pinLight, _invertRelay ? LOW : HIGH);
			}
			else
			{
				digitalWrite(_pinLight, _invertRelay ? HIGH : LOW);
			}
		}
		else
		{
			digitalWrite(_pinLight, _invertRelay ? HIGH : LOW);
		}
		_lastLightTime = currentMillis;
	}
}

void HandleTemps(unsigned long currentMillis)
{
	if (currentMillis - _lastReadTime > _readInterval)
	{
		_tempInternal = ReadInternalTempF();

		if (_tempInternal < _lowTemp)
		{
			_lowTemp = _tempInternal;
		}

		//WriteToDebug();

		_checkTemp++;

		if (_checkTemp >= _checkTempInterval)
		{
			_checkTemp = 0;
			if (_heat)
			{
				_heat = _tempInternal < _targetInternalTemp;
			}
			else
			{
				_heat = _tempInternal <= _minInternalTemp;
			}

			if (_invertRelay)
			{
				digitalWrite(_pinHeat, _heat ? LOW : HIGH);
			}
			else
			{
				digitalWrite(_pinHeat, _heat ? HIGH : LOW);
			}
		}
		_lastReadTime = currentMillis;
	}
}

bool HasTime()
{
	time_t utc = time(nullptr);
	struct tm timeinfo;
	gmtime_r(&utc, &timeinfo);
	return timeinfo.tm_year > 100;
}

void UpdateTime()
{
	Serial.printf("Scanning for networks\n");
	Serial.println(WiFi.softAPmacAddress());
	WiFi.mode(WIFI_STA);
	auto numAP = WiFi.scanNetworks();
	vector<Network> openNets;
	Serial.printf("%d networks\n", (int)numAP);
	for (int i = 0; i < numAP; i++)
	{
		Network n;
		n.ssid = WiFi.SSID(i);
		n.channel = WiFi.channel(i);
		memcpy(n.bssid, WiFi.BSSID(i), 6);
		auto et = WiFi.encryptionType(i);
		Serial.printf("  %d: e=%d c=%d %s\n", i, (int)et, n.channel, n.ssid.c_str());
		if (et == WIFI_AUTH_OPEN && n.ssid.length() > 0)
		{
			openNets.push_back(n);
		}
	}
	Serial.printf("%d open networks\n", (int)openNets.size());

	for (size_t i = 0; i < openNets.size(); i++)
	{
		if (UpdateTimeFromNetwork(openNets[i]))
		{
			break;
		}
	}
	ShowTime();
}

bool UpdateTimeFromNetwork(struct Network &network)
{
	Serial.printf("Connecting to: %s c=%d\n", network.ssid.c_str(), network.channel);
	unsigned long connectTimeout = 20 * 1000;

	WiFi.begin(network.ssid.c_str(), nullptr, network.channel, network.bssid);

	auto status = WiFi.status();

	auto startTime = millis();
	// wait for connection, fail, or timeout
	while (status != WL_CONNECTED && status != WL_NO_SSID_AVAIL && status != WL_CONNECT_FAILED && (millis() - startTime) <= connectTimeout)
	{
		delay(200);
		Serial.printf(".");
		status = WiFi.status();
	}
	Serial.printf("\n%s status = %d\n", network.ssid.c_str(), (int)status);
	String url = "http://neverssl.com";
	if (status == 3)
	{
		return UpdateTimeFromHttpResponseHeader(url);
	}
	//WiFi.disconnect();
	return false;
}

bool UpdateTimeFromHttpResponseHeader(const String &url)
{
	MyHTTPClient http;
	http.begin(url.c_str());
	int httpCode = http.GET();

	auto date = http.getDate();
	if (date.length() > 0)
	{
		return UpdateTimeFromDateString(date);
	}

	auto location = http.getLocation();
	if (httpCode / 100 == 3 && location.length() > 0)
	{
		Serial.printf("Redirect to: %s\n", location.c_str());
		return UpdateTimeFromHttpResponseHeader(location);
	}

	return false;
}

void ShowTime()
{
	time_t utc = time(nullptr);
	struct tm timeinfo;
	gmtime_r(&utc, &timeinfo);
	Serial.print(F("Current time: "));
	Serial.print(asctime(&timeinfo));
}

bool UpdateTimeFromDateString(const String &date)
{
	Serial.printf("DATE: %s\n", date.c_str());

	struct tm timeinfo = {0};

	auto s = (char *)date.c_str();
	auto end = s + date.length();
	auto b = s;

	while (b < end && !isdigit(*b))
		b++;
	auto e = b + 1;
	while (e < end && isdigit(*e))
		e++;
	*e = 0;
	timeinfo.tm_mday = atoi(b);
	//  Serial.printf("DAY: %d\n", timeinfo.tm_mday);

	b = e + 1;
	while (b < end && !isalpha(*b))
		b++;
	e = b + 1;
	while (e < end && isalpha(*e))
		e++;
	*e = 0;
	if (stricmp(b, "feb") == 0)
		timeinfo.tm_mon = 1;
	else if (stricmp(b, "mar") == 0)
		timeinfo.tm_mon = 2;
	else if (stricmp(b, "apr") == 0)
		timeinfo.tm_mon = 3;
	else if (stricmp(b, "may") == 0)
		timeinfo.tm_mon = 4;
	else if (stricmp(b, "jun") == 0)
		timeinfo.tm_mon = 5;
	else if (stricmp(b, "jul") == 0)
		timeinfo.tm_mon = 6;
	else if (stricmp(b, "aug") == 0)
		timeinfo.tm_mon = 7;
	else if (stricmp(b, "sep") == 0)
		timeinfo.tm_mon = 8;
	else if (stricmp(b, "oct") == 0)
		timeinfo.tm_mon = 9;
	else if (stricmp(b, "nov") == 0)
		timeinfo.tm_mon = 10;
	else if (stricmp(b, "dec") == 0)
		timeinfo.tm_mon = 11;
	//  Serial.printf("MONTH: %d\n", timeinfo.tm_mon);

	b = e + 1;
	while (b < end && !isdigit(*b))
		b++;
	e = b + 1;
	while (e < end && isdigit(*e))
		e++;
	*e = 0;
	timeinfo.tm_year = atoi(b) - 1900;
	//  Serial.printf("YEAR: %d\n", timeinfo.tm_year);

	b = e + 1;
	while (b < end && !isdigit(*b))
		b++;
	e = b + 1;
	while (e < end && isdigit(*e))
		e++;
	*e = 0;
	timeinfo.tm_hour = atoi(b);

	b = e + 1;
	while (b < end && !isdigit(*b))
		b++;
	e = b + 1;
	while (e < end && isdigit(*e))
		e++;
	*e = 0;
	timeinfo.tm_min = atoi(b);

	b = e + 1;
	while (b < end && !isdigit(*b))
		b++;
	e = b + 1;
	while (e < end && isdigit(*e))
		e++;
	*e = 0;
	timeinfo.tm_sec = atoi(b);

	//  strptime (s, "%a, %d %m %Y %H:%M:%S GMT", &timeinfo);
	Serial.print(F("Updating to server time: "));
	Serial.print(asctime(&timeinfo));

	auto nowSec = mktime(&timeinfo);

	timeval tv{nowSec, 0};
	timezone tz = {0};
	settimeofday(&tv, &tz);
	return true;
}

void WriteToDebug()
{
	Serial.print("Internal: ");
	Serial.print(_tempInternal);
	Serial.print(" Target: ");
	Serial.print(_targetInternalTemp);
	Serial.print(" Heat: ");
	Serial.print(_heat ? "On  " : "Off ");
	Serial.print("Low: ");
	Serial.println(_lowTemp);
}

float ReadInternalTempC()
{
	float temp = _bmp.readTemperature();
	return temp;
}

float ReadInternalTempF()
{
	float temp = ReadInternalTempC();
	temp = temp * 1.8 + 32;

	return temp;
}

static int stricmp(char const *a, char const *b)
{
	for (;; a++, b++)
	{
		int d = tolower((unsigned char)*a) - tolower((unsigned char)*b);
		if (d != 0 || !*a)
			return d;
	}
}
