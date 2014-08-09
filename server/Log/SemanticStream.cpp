#include "SemanticStream.h"

std::string SemanticStream::ToString() const
{
	bool last_entry_has_description = false;
	std::ostringstream os;
	for (auto it = lexical_entries.begin(); it != lexical_entries.end(); ++it)
	{
		if (it->has_name())
		{
			if (last_entry_has_description)
			{
				os << ", "; //连续的带名字项用逗号隔开
			}
			else
			{
				last_entry_has_description = true;
			}
			os << it->name.v << " = ";
		}
		else
		{
			last_entry_has_description = false;
		}

		switch (it->value_type)
		{
			case LexicalEntryValue::int_value_type:
				os << (int64_t)it->ivalue;
				break;
			case LexicalEntryValue::uint_value_type:
				os << it->ivalue;
				break;
			case LexicalEntryValue::string_value_type:
				os << it->svalue;
				break;
#ifdef WIN32
			case LexicalEntryValue::wstring_value_type:
				os << LexicalEntry::UnicodeToAscii(LexicalEntry::UTF8ToUnicode(it->svalue));
				break;
#endif
			case LexicalEntryValue::uint16_value_type_display_as_hex:
				os << "0x" << std::hex << (uint16_t)it->ivalue << std::dec;
				break;
			case LexicalEntryValue::uint_value_type_display_as_hex:
				os << "0x" << std::hex << it->ivalue << std::dec;
				break;
			case LexicalEntryValue::bytes_value_type_no_display:						
			case LexicalEntryValue::bytes_value_type_display_as_hex:
				os << "{ bytes length: " << it->bytes_length << " }";
				if (it->value_type == LexicalEntryValue::bytes_value_type_display_as_hex)
				{
					os << std::endl;
					#define BYTES_PER_LINE (16)
					char buf[4];
					buf[3] = 0;
					for (int i = 0; i < it->bytes_length; ++i)
					{
						sprintf(buf, " %02x", (uint8_t)it->bytes[i]);
						os << buf;
						if (i % BYTES_PER_LINE == BYTES_PER_LINE - 1 && i != it->bytes_length - 1) //最后一行不打换行
						{
							os << std::endl;
						}
					}
				}
				break;
			case LexicalEntryValue::double_value_type:
				os << it->dvalue;
				break;
			default:
				break;
		}
	}
	return os.str();
}