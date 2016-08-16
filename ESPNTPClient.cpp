#include <NtpClientLib.h>
#include <ESPNTPClient.h>

#ifdef ARDUINO_ARCH_ESP8266

#define DBG_PORT Serial

#ifdef DEBUG_NTPCLIENT
#define DEBUGLOG(...) DBG_PORT.printf(__VA_ARGS__)
#else
#define DEBUGLOG(...)
#endif


ESPNTPClient NTP;

ESPNTPClient::ESPNTPClient()
{
}

bool ESPNTPClient::setNtpServerName(int idx, String ntpServerName)
{
	char * buffer = (char *)malloc((ntpServerName.length()+1) * sizeof(char));
	if ((idx >= 0) && (idx <= 2)) {
		sntp_stop();
		ntpServerName.toCharArray(buffer, ntpServerName.length()+1);
		sntp_setservername(idx, buffer);
		DEBUGLOG("NTP server %d set to: %s \r\n", idx, buffer);
		sntp_init();
		return true;
	}
	return false;
}

String ESPNTPClient::getNtpServerName(int idx)
{
	if ((idx >= 0) && (idx <= 2)) {
		return String(sntp_getservername(idx));
	}
	return "";
}

bool ESPNTPClient::setTimeZone(int timeZone)
{
	//if ((timeZone >= -11) && (timeZone <= 13)) {
	sntp_stop();
	bool result = sntp_set_timezone(timeZone);
	sntp_init();
	DEBUGLOG("NTP time zone set to: %d, result: %s\r\n", timeZone, result?"OK":"error");
	return result;
	//return true;
	//}
	//return false;
}

int ESPNTPClient::getTimeZone()
{
	return sntp_get_timezone();
}

void ESPNTPClient::setLastNTPSync(time_t moment) {
	_lastSyncd = moment;
}

time_t getTime()
{
	DEBUGLOG("-- NTP Server hostname: %s\r\n", sntp_getservername(0));
	DEBUGLOG("-- Transmit NTP Request\r\n");
	uint32 secsSince1970 = sntp_get_current_timestamp();
	NTP.getUptime();
	if (secsSince1970) {
		setSyncInterval(NTP.getInterval()); // Normal refresh frequency
		DEBUGLOG("Sync frequency set low\r\n");
		if (NTP.getDayLight()) {
			if (NTP.summertime(year(secsSince1970), month(secsSince1970), day(secsSince1970), hour(secsSince1970), NTP.getTimeZone())) {
				secsSince1970 += SECS_PER_HOUR;
				DEBUGLOG("Summer Time\r\n");
			}
			else {
				DEBUGLOG("Winter Time\r\n");
			}
	
		}
		else {
			DEBUGLOG("No daylight\r\n");

		}
		NTP.getFirstSync();
		NTP.setLastNTPSync(secsSince1970);
		DEBUGLOG("Succeccful NTP sync at %s\r\n", NTP.getTimeDateString(NTP.getLastNTPSync()).c_str());
	}
	else {
		DEBUGLOG("-- NTP error :-(\r\n");
		setSyncInterval(NTP.getShortInterval()); // Fast refresh frequency, until successful sync
	}

	return secsSince1970;
}

boolean ESPNTPClient::begin(String ntpServerName, int timeOffset, boolean daylight)
{
	if (!setNtpServerName(0, ntpServerName)) {
		return false;
	}
	if (!setTimeZone(timeOffset)) {
		return false;
	}
	sntp_init();
	setDayLight(daylight);
	_lastSyncd = 0;

	if (!setInterval(DEFAULT_NTP_SHORTINTERVAL, DEFAULT_NTP_INTERVAL)) {
		return false;
	}
	DEBUGLOG("Time sync started\r\n");

	setSyncInterval(getShortInterval());
	setSyncProvider(getTime);

	return true;
}

boolean ESPNTPClient::stop() {
	setSyncProvider(NULL);
	DEBUGLOG("Time sync disabled\r\n");
	sntp_stop();
	return true;
}

boolean ESPNTPClient::setInterval(int interval)
{
	if (interval >= 10) {
		if (_longInterval != interval) {
			_longInterval = interval;
			DEBUGLOG("Long sync interval set to %d", interval);
			if (timeStatus() != timeSet)
				setSyncInterval(interval);
		}
		return true;
	}
	DEBUGLOG("Error setting interval %d\r\n", interval);

	return false;
}

boolean ESPNTPClient::setInterval(int shortInterval, int longInterval) {
	if (shortInterval >= 10 && longInterval >= 10) {
		_shortInterval = shortInterval;
		_longInterval = longInterval;
		if (timeStatus() != timeSet) {
			setSyncInterval(shortInterval);
		}
		else {
			setSyncInterval(longInterval);
		}
		DEBUGLOG("Short sync interval set to %d\r\n", shortInterval);
		DEBUGLOG("Long sync interval set to %d\r\n", longInterval);
		return true;
	}
	DEBUGLOG("Error setting interval. Short: %d Long: %d\r\n", shortInterval, longInterval);
	return false;
}

int ESPNTPClient::getInterval()
{
	return _longInterval;
}

int ESPNTPClient::getShortInterval()
{
	return _shortInterval;
}

void ESPNTPClient::setDayLight(boolean daylight)
{
	_daylight = daylight;
	DEBUGLOG("--Set daylight %s", daylight? "ON" : "OFF");
}

boolean ESPNTPClient::getDayLight()
{
	return _daylight;
}

String ESPNTPClient::getTimeStr(time_t moment) {
	if ((timeStatus() != timeNotSet) || (moment != 0)) {
		String timeStr = "";
		timeStr += printDigits(hour(moment));
		timeStr += ":";
		timeStr += printDigits(minute(moment));
		timeStr += ":";
		timeStr += printDigits(second(moment));

		return timeStr;
	}
	else return "Time not set";
}

String ESPNTPClient::getTimeStr() {
	return getTimeStr(now());
}

String ESPNTPClient::getDateStr(time_t moment) {
	if ((timeStatus() != timeNotSet) || (moment != 0)) {
		String timeStr = "";

		timeStr += printDigits(day(moment));
		timeStr += "/";
		timeStr += printDigits(month(moment));
		timeStr += "/";
		timeStr += String(year(moment));

		return timeStr;
	}
	else return "Date not set";
}

String ESPNTPClient::getDateStr() {
	return getDateStr(now());
}

String ESPNTPClient::getTimeDateString(time_t moment) {
	if ((timeStatus() != timeNotSet) || (moment != 0)) {
		String timeStr = "";
		timeStr += getTimeStr(moment);
		timeStr += " ";
		timeStr += getDateStr(moment);

		return timeStr;
	}
	else {
		return "Time not set";
	}
}

String ESPNTPClient::getTimeDateString() {
	if (timeStatus() == timeSet) {
		return getTimeDateString(now());
	}
	return "Time not set";
}

String ESPNTPClient::printDigits(int digits) {
	// utility for digital clock display: prints preceding colon and leading 0
	String digStr = "";

	if (digits < 10)
		digStr += '0';
	digStr += String(digits);

	return digStr;
}

boolean ESPNTPClient::summertime(int year, byte month, byte day, byte hour, byte tzHours)
// input parameters: "normal time" for year, month, day, hour and tzHours (0=UTC, 1=MEZ)
{
	if ((month<3) || (month>10)) return false; // keine Sommerzeit in Jan, Feb, Nov, Dez
	if ((month>3) && (month<10)) return true; // Sommerzeit in Apr, Mai, Jun, Jul, Aug, Sep
	if (month == 3 && (hour + 24 * day) >= (1 + tzHours + 24 * (31 - (5 * year / 4 + 4) % 7)) || month == 10 && (hour + 24 * day)<(1 + tzHours + 24 * (31 - (5 * year / 4 + 1) % 7)))
		return true;
	else
		return false;
}

time_t ESPNTPClient::getUptime()
{
	_uptime = _uptime + (millis() - _uptime);
	return _uptime / 1000;
}

time_t ESPNTPClient::getFirstSync()
{
	if (!_firstSync) {
		if (timeStatus() == timeSet) {
			_firstSync = now() - getUptime();
		}
	}
	
	return _firstSync;
}

String ESPNTPClient::getUptimeString() {
	uint days;
	uint8 hours;
	uint8 minutes;
	uint8 seconds;
	
	long uptime = getUptime();

	seconds = uptime % SECS_PER_MIN;
	uptime -= seconds;
	minutes = (uptime % SECS_PER_HOUR)/ SECS_PER_MIN;
	uptime -= minutes * SECS_PER_MIN;
	hours = (uptime % SECS_PER_DAY) / SECS_PER_HOUR;
	uptime -= hours * SECS_PER_HOUR;
	days = uptime / SECS_PER_DAY;

	String uptimeStr = ""; 
	char buffer[20];
	sprintf(buffer, "%4d days %02d:%02d:%02d", days, hours, minutes, seconds);
	uptimeStr += buffer;

	return uptimeStr;
}

time_t ESPNTPClient::getLastNTPSync() {
	return _lastSyncd;
}

#endif // ARDUINO_ARCH_ESP8266