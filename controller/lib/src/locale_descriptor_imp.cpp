/*
 * Licensed under the MIT License (MIT)
 *
 * Copyright (c) 2013 AudioScience Inc.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy of
 * this software and associated documentation files (the "Software"), to deal in
 * the Software without restriction, including without limitation the rights to
 * use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of
 * the Software, and to permit persons to whom the Software is furnished to do so,
 * subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS
 * FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR
 * COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
 * IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */

/**
 * locale_descriptor_imp.cpp
 *
 * Locale descriptor implementation
 */

#include "enumeration.h"
#include "log_imp.h"
#include "locale_descriptor_imp.h"

namespace avdecc_lib
{
	locale_descriptor_imp::locale_descriptor_imp() {}

	locale_descriptor_imp::locale_descriptor_imp(end_station_imp *base_end_station_imp_ref, uint8_t *frame, size_t pos, size_t mem_buf_len) : descriptor_base_imp(base_end_station_imp_ref)
	{
		desc_locale_read_returned = jdksavdecc_descriptor_locale_read(&locale_desc, frame, pos, mem_buf_len);

		if(desc_locale_read_returned < 0)
		{
			log_imp_ref->post_log_msg(LOGGING_LEVEL_ERROR, "desc_locale_read error");
			assert(desc_locale_read_returned >= 0);
		}
	}

	locale_descriptor_imp::~locale_descriptor_imp() {}

	uint16_t STDCALL locale_descriptor_imp::get_descriptor_type()
	{
		assert(locale_desc.descriptor_type == JDKSAVDECC_DESCRIPTOR_LOCALE);
		return locale_desc.descriptor_type;
	}

	uint16_t STDCALL locale_descriptor_imp::get_descriptor_index()
	{
		return locale_desc.descriptor_index;
	}

	uint8_t * STDCALL locale_descriptor_imp::get_locale_identifier()
	{
		return locale_desc.locale_identifier.value;
	}

	uint16_t STDCALL locale_descriptor_imp::get_number_of_strings()
	{
		return locale_desc.number_of_strings;
	}

	uint16_t STDCALL locale_descriptor_imp::get_base_strings()
	{
		return locale_desc.base_strings;
	}
}
