#pragma once

namespace LexicalEntryValue
{
	enum value_type_enum
	{
		int_value_type,
		uint_value_type,
		uint_value_type_display_as_hex,
		uint16_value_type_display_as_hex,
		string_value_type,
		bytes_value_type_no_display,
		bytes_value_type_display_as_hex,
		double_value_type,
		//新增enum值不能插中间，否则以前的数据就都失效了。必须加后面
		wstring_value_type,
		undefined_value_type = 100 //12.08.01增加，主要用于错误情况下的默认值
	};
}