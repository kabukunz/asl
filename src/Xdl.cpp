#include <asl/Xdl.h>
#include <asl/TextFile.h>
#include <stdio.h>
#include <ctype.h>
#include <locale.h>

#if defined(_MSC_VER) && _MSC_VER < 1800
#include <float.h>
#endif

#ifdef ASL_FAST_JSON
#define ASL_ATOF myatof
#define FLOAT_F "%.8g"
#define DOUBLE_F "%.16g"
#else
#define ASL_ATOF atof
#define FLOAT_F "%.9g"
#define DOUBLE_F "%.17g"
#endif

namespace asl {

enum StateN {
	NUMBER, INT, STRING, PROPERTY, IDENTIFIER,
	NUMBER_E, NUMBER_ES, NUMBER_EV, NUMBER_DOT, MINUS, WAIT_SEP,
	WAIT_EQUAL, WAIT_VALUE, WAIT_PROPERTY, WAIT_OBJ, QPROPERTY, ESCAPE, ERR, UNICODECHAR
};
enum ContextN {ROOT, ARRAY, OBJECT, COMMENT1, COMMENT, LINECOMMENT, ENDCOMMENT};

#define INDENT_CHAR '\t'

Var decodeXDL(const String& xdl)
{
	XdlParser parser;
	return parser.decode(xdl);
}

Var decodeJSON(const char* json)
{
	XdlParser parser;
	return parser.decode(json);
}

String encodeXDL(const Var& data, bool pretty, bool json)
{
	XdlEncoder encoder;
	return encoder.encode(data, pretty, json);
}

Var decodeJSON(const String& json)
{
	return decodeXDL(json);
}

String encodeJSON(const Var& data, bool pretty)
{
	return encodeXDL(data, pretty, true);
}

Var Xdl::read(const String& file)
{
	return decodeXDL(TextFile(file).text());
}

bool Xdl::write(const String& file, const Var& v, bool pretty)
{
	return TextFile(file).put(encodeXDL(v, pretty));
}

Var Json::read(const String& file)
{
	return decodeJSON(TextFile(file).text());
}

bool Json::write(const String& file, const Var& v, bool pretty)
{
	return TextFile(file).put(encodeJSON(v, pretty));
}


inline void XdlParser::value_end()
{
	_state = (_context.top() == ROOT) ? WAIT_VALUE : WAIT_SEP;
	_buffer="";
}

void XdlParser::reset()
{
	_context.clear();
	_context << ROOT;
	_state = WAIT_VALUE;
	_buffer = "";
}

void XdlParser::parse(const char* s)
{
	if(_state == ERR)
		return;
	while(char c=*s++)
	{
		Context ctx = _context.top();
		if(!_inComment)
		{
			if(c=='/' && _state != STRING && _state != ESCAPE)
			{
				_inComment = true;
				_context << COMMENT1;
				continue;
			}
			goto NOCOMMENT;
		}
		switch(ctx)
		{
		case COMMENT1:
			_context.pop();
			if (c == '/')
				_context << LINECOMMENT;
			else if (c == '*')
				_context << COMMENT;
			else
				_state = ERR;
			break;
		case LINECOMMENT:
			if(c=='\n' || c=='\r')
			{
				_inComment = false;
				_context.pop();
			}
			break;
		case COMMENT:
			if(c=='*')
				_context << ENDCOMMENT;
			break;
		case ENDCOMMENT:
			_context.pop();
			if(c=='/') {
				_inComment = false;
				_context.pop();
				continue;
			}
			break;
		default:
			if(c=='/' && _state != STRING)
			{
				_inComment = true;
				_context << COMMENT1;
			}
			continue;
		}
		ctx = _context.top();
		if(ctx==COMMENT || ctx==LINECOMMENT || ctx==COMMENT1 || ctx==ENDCOMMENT)
		{
			_inComment = true;
			continue;
		}
		else
			_inComment = false;
		NOCOMMENT:
		switch(_state)
		{
		case MINUS:
			if (c >= '0' && c <= '9')
			{
				_state = INT;
				_buffer << c;
			}
			else
			{
				_state = ERR;
				return;
			}
			break;
		case INT:
			if(c>='0' && c<='9')
			{
				_buffer << c;
			}
			else if(c=='.')
			{
				_state = NUMBER_DOT;
				_buffer << c;
			}
			else if (c == 'e' || c == 'E')
			{
				_state = NUMBER_E;
				_buffer << c;
			}
			else if(_buffer != '-')
			{
				if (_buffer[0] == '-') // this block to check starting zero !
				{
					if (_buffer[1] == '0' && _buffer[2] != '\0')
						_state = ERR;
				}
				else if (_buffer[0] == '0' && _buffer[1] != '\0')
					_state = ERR;
				if (_state == ERR)
					return;

				if (_buffer.length() > 9) // check better if it fits in an int32
					new_number(ASL_ATOF(_buffer));
				else
					new_number(myatoiz(_buffer));
				value_end();
				s--;
			}
			else
			{
				_state = ERR;
				return;
			}
			break;
		case NUMBER_DOT:
			if (c >= '0' && c <= '9')
			{
				_state = NUMBER;
				_buffer << c;
			}
			else
			{
				_state = ERR;
				return;
			}
			break;
		case NUMBER_E:
			if (c == '-' || c == '+')
			{
				_state = NUMBER_ES;
				_buffer << c;
			}
			else if (c >= '0' && c <= '9')
			{
				_state = NUMBER_EV;
				_buffer << c;
			}
			else
			{
				_state = ERR;
				return;
			}
			break;
		case NUMBER_ES:
			if (c >= '0' && c <= '9')
			{
				_state = NUMBER_EV;
				_buffer << c;
			}
			else
			{
				_state = ERR;
				return;
			}
			break;
		case NUMBER_EV:
			if (c >= '0' && c <= '9')
			{
				_buffer << c;
			}
			else if (c == ',' || myisspace(c) || c == ']' || c == '}')
			{
#ifndef ASL_FAST_JSON
				for (char* p = _buffer; *p; p++)
					if (*p == '.')
					{
						*p = _ldp;
						break;
					}
#endif
				new_number(ASL_ATOF(_buffer));
				value_end();
				s--;
			}
			else
			{
				_state = ERR;
				return;
			}
			break;
		case NUMBER:
			if(c >= '0' && c <= '9')
			{
				_buffer << c;
			}
			else if (c == 'e' || c == 'E')
			{
				_state = NUMBER_E;
				_buffer << c;
			}
			else if(c == ',' || myisspace(c) || c == ']' || c == '}')
			{
#ifndef ASL_FAST_JSON
				for(char* p = _buffer; *p; p++)
					if (*p == '.')
					{
						*p = _ldp;
						break;
					}
#endif
				new_number(ASL_ATOF(_buffer));
				value_end();
				s--;
			}
			else
			{
				_state = ERR;
				return;
			}

			break;

		case STRING:
			if(c!='\"' && c!='\\')
				_buffer << c;
			else if(c=='\\')
			{
				_state=ESCAPE;
			}
			else // disallow TAB and newline?
			{
				new_string(_buffer);
				value_end();
			}
			break;

		case PROPERTY:
			//if(!isalnum(c) && c!='_')
			if (c == '=' || myisspace(c))
			{
				new_property(_buffer);
				s--;
				_state=WAIT_EQUAL;
				_buffer="";
			}
			else // not checking possible identifier chars !
				_buffer << c;
			break;
		case QPROPERTY: // JSON
			if(c !='"')
				_buffer << c;
			else
			{
				new_property(_buffer);
				//s--;
				_state=WAIT_EQUAL;
				_buffer="";
			}
			break;

		case WAIT_VALUE:
			if (c >= '0' && c <= '9')
			{
				_state = INT;
				_buffer << c;
			}
			else if (c == '-')
			{
				_state = MINUS;
				_buffer << c;
			}
			else if (c == '\"')
			{
				_state = STRING;
			}
			else if (c == '[')
			{
				begin_array();
				_context << ARRAY;
			}
			else if(c=='{')
			{
				begin_object(_buffer);
				_state=WAIT_PROPERTY;
				_context << OBJECT;
				_buffer="";
			}
			/*else if(c=='.') // not JSON
			{
				state=NUMBER;
				buffer += c;
			}*/
			else if (c == '}' && ctx == OBJECT) // only if we allow {}
			{
				_context.pop();
				value_end();
				end_object();
			}
			else if (myisalnum(c) || c == '_' || c == '$')
			{
				_state=IDENTIFIER;
				_buffer << c;
			}
			else if(c==']' && ctx==ARRAY)
			{
				/*if ((*(Array<Var>*)_lists.top().list).length() != 0)
				{
					_state = ERR;
					return;
				}*/
				_context.pop();
				value_end();
				end_array();
			}
			else if(!myisspace(c) /*&& c != ','*/)
			{
				_state = ERR;
				return;
			}
			break;
		case WAIT_SEP:
			if (c == ',' || c == '\n')
				_state = (ctx == OBJECT) ? WAIT_PROPERTY : WAIT_VALUE;
			else if (c == '}' && ctx == OBJECT)
			{
				_context.pop();
				value_end();
				end_object();
			}
			else if (c == ']' && ctx == ARRAY)
			{
				_context.pop();
				value_end();
				end_array();
			}
			else if (!myisspace(c))
			{
				_state = ERR;
				return;
			}
			break;
		case WAIT_OBJ:
			if (c == '{')
			{
				begin_object(_buffer);
				_state = WAIT_PROPERTY;
				_context << OBJECT;
				_buffer = "";
			}
			else if (!myisspace(c))
			{
				_state = ERR;
				return;
			}
			break;
		case WAIT_PROPERTY:
			if(myisalnum(c)||c=='_'||c=='$')
			{
				_state=PROPERTY;
				_buffer << c;
			}
			else if(c=='"') // JSON
			{
				_state=QPROPERTY;
			}
			else if(c=='}')
			{
				_context.pop();
				value_end();
				end_object();
			}
			else if(!myisspace(c) /*&& c != ','*/ && c != '}')
			{
				_state = ERR;
				return;
			}
			break;
		case ESCAPE:
			if (c == '\\')
				_buffer << '\\';
			else if (c == '"')
				_buffer << '"';
			else if (c == 'n')
				_buffer << '\n';
			else if (c == '/')
				_buffer << '/';
			else if (c == 'r')
				_buffer << '\r';
			else if (c == 't')
				_buffer << '\t';
			else if (c == 'f')
				_buffer << '\f';
			else if (c == 'b')
				_buffer << '\b';
			else if (c == 'u')
			{
				_state = UNICODECHAR;
				_unicodeCount = 0;
				break;
			}
			else
			{
				_state = ERR;
				break;
			}
			_state=STRING;
			break;

		case IDENTIFIER:
			if(!myisalnum(c) && c!='_')
			{
				if(_buffer=="Y" || _buffer=="N" || _buffer=="false" || _buffer=="true" )
				{
					new_bool(_buffer=="true"||_buffer=="Y");
					value_end();
				}
				else if(_buffer=="null")
				{
					put(Var::NUL);
					value_end();
				}
				else
					_state = WAIT_OBJ;
				s--;
			}
			else
				_buffer << c;
			break;
		case WAIT_EQUAL:
			if(c=='=' || c==':')
				_state=WAIT_VALUE;
			else if(!myisspace(c))
			{
				_state = ERR;
				return;
			}
			break;
		case UNICODECHAR:
			_unicode[_unicodeCount++] = c;
			if(_unicodeCount == 4) // still only BMP!
			{
				_unicode[4] = '\0';
				wchar_t wch[2] = {(wchar_t)strtoul(_unicode, NULL, 16), 0};
				char ch[4];
				utf16toUtf8(wch, ch, 1);
				_buffer << ch;
				_state=STRING;
			}
			break;
		case ERR:
			break;
		}
//		printf("%c %i\n", c, state);
	}
}

XdlParser::XdlParser()
{
	lconv* loc = localeconv();
	_ldp = *loc->decimal_point;
	_context << ROOT;
	_state = WAIT_VALUE;
	_inComment = false;
	_lists << Container(Var::ARRAY);
	_lists.top().init();
}

XdlParser::~XdlParser()
{
	while(_lists.length() > 0)
	{
		_lists.top().free();
		_lists.pop();
	}
}

Var XdlParser::value() const
{
	Var v;
	if (_state == ERR)
		return v;
	Array<Var>& l = *(Array<Var>*)_lists[0].list;
	if(_context.top() == ROOT && _state == WAIT_VALUE && l.length() > 0)
		v = l[l.length()-1];
	return v;
}

Var XdlParser::decode(const char* s)
{
	parse(s);
	parse(" ");
	return value();
}


void XdlParser::begin_array()
{
	_lists << Container(Var::ARRAY);
	_lists.top().init();
}

void XdlParser::end_array()
{
	Var v = *(Array<Var>*)_lists.top().list;
	_lists.top().free();
	_lists.pop();
	put(v);
}

void XdlParser::begin_object(const char* _class)
{
	_lists << Container(Var::DIC);
	_lists.top().init();
	if(_class[0] != '\0')
		(*(HDic<Var>*)_lists.top().dict)[ASL_XDLCLASS]=_class;
}

void XdlParser::end_object()
{
	Var v = *(HDic<Var>*)_lists.top().dict;
	_lists.top().free();
	_lists.pop();
	put(v);
}

void XdlParser::new_property(const char* name)
{
	_props << (String)name;
}

void XdlParser::new_property(const String& name)
{
	_props << name;
}

void XdlParser::put(const Var& x)
{
	Container& top = _lists.top();
	switch(top.type)
	{
	case Var::ARRAY:
		(*(Array<Var>*)top.list) << x;
		break;
	case Var::DIC: {
		const String& name = _props.top();
		(*(HDic<Var>*)top.dict).set(name, x);
		_props.pop();
		break;
	}
	default: break;
	}
}

XdlWriter::XdlWriter()
{
	_pretty = false;
	_json = false;
	_out.resize(512);
}

void XdlEncoder::_encode(const Var& v)
{
	switch(v._type)
	{
	case Var::FLOAT:
		new_number((float)v.d);
		break;
	case Var::NUMBER:
		new_number(v.d);
		break;
	case Var::INT:
		new_number(v.i);
		break;
	case Var::STRING:
		new_string((*v.s).ptr());
		break;
	case Var::SSTRING:
		new_string(v.ss);
		break;
	case Var::BOOL:
		new_bool(v.b);
		break;
	case Var::ARRAY: {
		begin_array();
		int n = v.length();
		const Var& v0 = n>0? v[0] : v;
		bool multi = (_pretty && (n > 10 || (n>0  && (v0.is(Var::ARRAY) || v0.is(Var::DIC)))));
		bool big = n>0 && (v0.is(Var::ARRAY) || v0.is(Var::DIC));
		if(multi)
		{
			_indent = String(INDENT_CHAR, ++_level);
			_out << '\n' << _indent;
			//linestart = out.length;
		}
		for(int i=0; i<v.length(); i++)
		{
			if(i>0) {
				if(multi) {
					if(big || (i%16)==0) {
						if(_json) _out << ',';
						_out << '\n' << _indent;
					}
					else put_separator();
				}
				else
					put_separator();
			}
			_encode(v[i]);
		}
		if(multi) {
			_indent = String(INDENT_CHAR, --_level);
			_out << '\n' << _indent;
		}
		end_array();
		//if(multi) out += '\n';
		break;
		}
	case Var::DIC: {
		bool hasclass = v.has(ASL_XDLCLASS);
		if(hasclass)
			begin_object(v[ASL_XDLCLASS].toString());
		else
			begin_object("");
		int k = (hasclass && _json)?1:0;
		_indent = String(INDENT_CHAR, ++_level);
		foreach2(String& name, Var& value, v)
		{
			if(!value.is(Var::NONE) && name!=ASL_XDLCLASS)
			{
				if(_pretty) {
					if(_json && k++!=0) _out << ',';
					_out << '\n' << _indent;
				}
				else if(k++>0) put_separator();
				new_property(name);
				_encode(value);
			}
		}
		if(_pretty) {
			_indent = String(INDENT_CHAR, --_level);
			_out << '\n' << _indent;
		}
		end_object();
		//if(pretty) out << '\n';
		}
		break;
	case Var::NUL:
		_out << "null";
		break;
	case Var::NONE:
		_out << "null";
		break;
	}
}

void XdlWriter::put_separator()
{
	_out << ',';
}

void XdlWriter::reset()
{
	_out = "";
}

void XdlWriter::new_number(int x)
{
	int n = _out.length();
	_out.resize(n+11);
	_out.fix(n + myitoa(x, &_out[n]));
}

void XdlWriter::new_number(double x)
{
	int n = _out.length();
#if defined(_MSC_VER) && _MSC_VER < 1800
	if (!_finite(x))
#else
	if (!isfinite(x))
#endif
	{
		if (x != x)
			_out << "null";
		else
			_out << ((x < 0)? "-1e400" : "1e400");
		return;
	}
	_out.resize(n+26);
	_out.fix(n + sprintf(&_out[n], DOUBLE_F, x));

	// Fix decimal comma of some locales
#ifndef ASL_NO_FIX_DOT
	char* p = &_out[n];
	while (*p)
	{
		if (*p == ',') {
			*p = '.';
			break;
		}
		p++;
	}
#endif
}

void XdlWriter::new_number(float x)
{
	int n = _out.length();
#if defined(_MSC_VER) && _MSC_VER < 1800
	if (!_finite(x))
#else
	if (!isfinite(x))
#endif
	{
		if (x != x)
			_out << "null";
		else
			_out << ((x < 0) ? "-1e400" : "1e400");
		return;
	}
	_out.resize(n + 16);
	_out.fix(n + sprintf(&_out[n], FLOAT_F, x));

	// Fix decimal comma of some locales
#ifndef ASL_NO_FIX_DOT
	char* p = &_out[n];
	while (*p)
	{
		if (*p == ',') {
			*p = '.';
			break;
		}
		p++;
	}
#endif
}

void XdlWriter::new_string(const char* x)
{
	_out << '\"';
	const char* p = x;
	while(char c = *p++) // optimizable: not appending chars directly
	{
		if(c=='\\')
			_out << "\\\\";
		else if (c == '\"')
			_out << "\\\"";
		else if (c == '\n')
			_out << "\\n";
		else if (c == '\r')
			_out << "\\r";
		else if (c == '\t')
			_out << "\\t";
		else if (c == '\f')
			_out << "\\f";
		else
			_out << c;
	}
	_out << '\"';
}


void XdlWriter::new_bool(bool x)
{
	if (_json)
		_out << (x ? "true" : "false");
	else
		_out << (x ? "Y" : "N");
}

void XdlWriter::begin_array()
{
	_out << '[';
}

void XdlWriter::end_array()
{
	_out << ']';
}

void XdlWriter::begin_object(const char* _class)
{
	if(!_json)
		_out << _class;
	_out << '{';
	if(_json && _class[0] != '\0')
		_out << "\"" ASL_XDLCLASS "\":\"" << _class << "\"";
}

void XdlWriter::end_object()
{
	_out << '}';
}

void XdlWriter::new_property(const char* name)
{
	if(_json)
		_out << '\"' << name << "\":";
	else
		_out << name << '=';
}

void XdlWriter::new_property(const String& name)
{
	if(_json)
		_out << '\"' << name << "\":";
	else
		_out << name << '=';
}

}
