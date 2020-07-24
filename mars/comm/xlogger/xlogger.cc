// Copyright (C) 2016 THL A29 Limited, a Tencent company. All rights reserved.

// Licensed under the MIT License (the "License"); you may not use this file except in
// compliance with the License. You may obtain a copy of the License at
// http://opensource.org/licenses/MIT

// Unless required by applicable law or agreed to in writing, software distributed under the License is
// distributed on an "AS IS" basis, WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND,
// either express or implied. See the License for the specific language governing permissions and
// limitations under the License.

/*
 ============================================================================
 Name		: xlogger.h
 ============================================================================
 */

#include "xlogger.h"

XLogger::~XLogger() {
	if (!m_isassert && m_message.empty()) return;

	gettimeofday(&m_info.timeval, NULL);
	if (m_hook && !m_hook(m_info, m_message)) return;

	xlogger_filter_t filter = xlogger_GetFilter();
	if (filter && filter(&m_info, m_message.c_str()) <= 0)  return;

	if (m_isassert)
		xlogger_Assert(m_isinfonull?NULL:&m_info, m_exp, m_message.c_str());
	else
		xlogger_Write(m_isinfonull?NULL:&m_info, m_message.c_str());
}

void XLogger::operator>> (XLogger& _xlogger) {
	if (_xlogger.m_info.level < m_info.level)
	{
		_xlogger.m_info.level = m_info.level;
		_xlogger.m_isassert = m_isassert;
		_xlogger.m_exp = m_exp;
	}

	m_isassert = false;
	m_exp = NULL;

	_xlogger.m_message += m_message;
	m_message.clear();
}

void XLogger::operator<< (XLogger& _xlogger) {
	_xlogger.operator>>(*this);
}

XLogger& XLogger::Assert(const char* _exp) {
	m_isassert = true;
	m_exp = _exp;
	return *this;
}

XLogger& XLogger::operator<< (const string_cast& _value) {
	if (NULL != _value.str()) {
		m_message += _value.str();
	} else {
		m_info.level = kLevelFatal;
		m_message += "{!!! XLogger& XLogger::operator<<(const string_cast& _value): _value.str() == NULL !!!}";
		assert(false);
	}
	return *this;
}

XLogger& XLogger::operator>>(const string_cast& _value) {
	if (NULL != _value.str()) {
		m_message.insert(0,  _value.str());
	} else {
		m_info.level = kLevelFatal;
		m_message.insert(0,  "{!!! XLogger& XLogger::operator>>(const string_cast& _value): _value.str() == NULL !!!}");
		assert(false);
	}
	return *this;
}

XLogger& XLogger::VPrintf(const char* _format, va_list _list) {
	if (_format == NULL)
	{
		m_info.level = kLevelFatal;
		m_message += "{!!! XLogger& XLogger::operator()(const char* _format, va_list _list): _format == NULL !!!}";
		assert(false);
		return *this;
	}

	char temp[4096] = {'\0'};
	vsnprintf(temp, 4096, _format, _list);
	m_message += temp;
	return *this;
}

XLogger& XLogger::operator()(const char* _format, ...) {
	if (_format == NULL)
	{
		m_info.level = kLevelFatal;
		m_message += "{!!! XLogger& XLogger::operator()(const char* _format, ...): _format == NULL !!!}";
		assert(false);
		return *this;
	}

	va_list valist;
	va_start(valist, _format);
	VPrintf(_format, valist);
	va_end(valist);
	return *this;
}

void XLogger::DoTypeSafeFormat(const char* _format, const string_cast** _args) {

	const char* current = _format;
	int count = 0;
	while ('\0' != *current)
	{
		if ('%' != *current)
		{
			m_message += *current;
			++current;
			continue;
		}

		char nextch = *(current+1);
		if (('0' <=nextch  && nextch <= '9') || nextch == '_')
		{

			int argIndex = count;
			if (nextch != '_') argIndex = nextch - '0';

			if (_args[argIndex] != NULL)
			{
				if (NULL != _args[argIndex]->str())
				{
					m_message += _args[argIndex]->str();
				} else {
					m_info.level = kLevelFatal;
					m_message += "{!!! void XLogger::DoTypeSafeFormat: _args[";
					m_message += string_cast(argIndex).str();
					m_message += "]->str() == NULL !!!}";
					assert(false);
				}
			} else {
				m_info.level = kLevelFatal;
				m_message += "{!!! void XLogger::DoTypeSafeFormat: _args[";
				m_message += string_cast(argIndex).str();
				m_message += "] == NULL !!!}";
				assert(false);
			}
			count++;
			current += 2;
		}
		else if (nextch == '%') {
			m_message += '%';
			current += 2;
		} else {
			++current;
			m_info.level = kLevelFatal;
			m_message += "{!!! void XLogger::DoTypeSafeFormat: %";
			m_message += nextch;
			m_message += " not fit mode !!!}";
			assert(false);
		}
	}
}
