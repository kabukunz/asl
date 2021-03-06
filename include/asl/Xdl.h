// Copyright(c) 1999-2020 aslze
// Licensed under the MIT License (http://opensource.org/licenses/MIT)

#ifndef ASL_XDL_H
#define ASL_XDL_H

#include <asl/Stack.h>
#include <asl/Array.h>
#include <asl/String.h>
#include <asl/Var.h>
#include <asl/JSON.h>

namespace asl {

/**
 * \defgroup XDL XML, XDL, and JSON
 * @{
 */

class ASL_API XdlCodec
{
protected:
	typedef char State;
	typedef char Context;
	State _state;
	Stack<Context> _context;
	String _buffer;
	bool _inComment;
	int _unicodeCount;
	char _unicode[5];
	char _ldp;
public:
	XdlCodec() {}
	virtual ~XdlCodec() {}
	virtual void new_number(int x) {}
	virtual void new_number(double x) {}
	virtual void new_string(const char* s) {}
	virtual void new_string(const String& x) {}
	virtual void new_bool(bool b) {}
	virtual void begin_array() {}
	virtual void end_array() {}
	virtual void begin_object(const char* c) {}
	virtual void end_object() {}
	virtual void new_property(const char* name) {}
	virtual void new_property(const String& name) {}
};

class ASL_API XdlParser: public XdlCodec
{
	struct Container
	{
		Var::Type type;
		union {
			char list[sizeof(Array<Var>)];
			char dict[sizeof(HDic<Var>)];
		};
		Container() {}
		Container(const Container& c) {memcpy(this, &c, sizeof(*this));}
		Container(Var::Type t) : type(t) {}
		void init() {
			if(type==Var::ARRAY)
				asl_construct((Array<Var>*)list);
			else
				asl_construct((HDic<Var>*)dict);
		}
		void free() {
			if(type==Var::ARRAY)
				asl_destroy((Array<Var>*)list);
			else
				asl_destroy((HDic<Var>*)dict);
		}
		//Container(Array<Var>* a): type(Var::ARRAY), list(a) {}
		//Container(HDic<Var>* d): type(Var::DIC), dict(d) {}
	};
	Stack<Container> _lists;
	Stack<String> _props;
	void put(const Var& x);
public:
	XdlParser();
	~XdlParser();
	void parse(const char* s);
	void value_end();
	virtual void reset();
	Var value() const;
	Var decode(const char* s);
	virtual void new_number(int x) {put(x);}
	virtual void new_number(double x) {put(x);}
	virtual void new_number(float x) { put(x); }
	virtual void new_string(const char* x) {put(x);}
	virtual void new_string(const String& x) {put(x);}
	virtual void new_bool(bool b) {put(b);}
	virtual void begin_array();
	virtual void end_array();
	virtual void begin_object(const char* _class);
	virtual void end_object();
	virtual void new_property(const char* name);
	virtual void new_property(const String& name);
};

class ASL_API XdlWriter: public XdlCodec
{
protected:
	String _out;
	bool _pretty, _json;
public:
	XdlWriter();
	~XdlWriter() {}
	String data() const {return _out;}
	void put_separator();
	virtual void reset();
	virtual void new_number(int x);
	virtual void new_number(double x);
	virtual void new_number(float x);
	virtual void new_string(const char* x);
	virtual void new_string(const String& x) {new_string(*x);}
	virtual void new_bool(bool b);
	virtual void begin_array();
	virtual void end_array();
	virtual void begin_object(const char* _class);
	virtual void end_object();
	virtual void new_property(const char* name);
	virtual void new_property(const String& name);
};

class ASL_API XdlEncoder: public XdlWriter
{
	String _indent;
	int _level;
	void _encode(const Var& v);
public:
	XdlEncoder() {_level=0;}
	String encode(const Var& v, bool p=false, bool j=false)
	{
		_pretty = p;
		_json = j;
		reset();
		_encode(v);
		return data();
	}
};


/*
Decodes the XDL-encoded string into a Var that will contain all the structure. If there are format parsing errors,
the result will be a `Var::NONE` typed variable.
\deprecated Use Xdl::decode()
*/
Var ASL_API decodeXDL(const String& xdl);

/*
Encodes the given Var into an XDL-format representation.
If parameter `pretty` is true, an indented representation is produced.
\deprecated Use Xdl::encode()
*/
String ASL_API encodeXDL(const Var& data, bool pretty = false, bool json = false);

/**
Static functions to encode/decode XDL data.

~~~
Var data = Xdl::decode("{x=1.5, y=[1,Y]}");  // decode XDL from a string
~~~
*/
struct ASL_API Xdl
{
	/**
	Reads and decodes data from a file in XDL format
	*/
	static Var read(const String& file);
	/**
	Writes a var to a file in XDL format
	*/
	static bool write(const String& file, const Var& v, bool pretty=true);
	/**
	Decodes the XDL-encoded string into a Var that will contain all the structure. If there are format parsing errors,
	the result will be a `Var::NONE` typed variable.
	*/
	static Var decode(const String& xdl) { return decodeXDL(xdl); }
	/**
	Encodes the given Var into an XDL-format representation.
	If parameter `pretty` is true, an indented representation is produced.
	*/
	static String encode(const Var& v, bool pretty = false) { return encodeXDL(v, pretty, false); }
};


/**@}*/

}

#endif
