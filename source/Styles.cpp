#include "notepad2.h"
#include "dialogs.h"
#include "styles.h"

extern CAppModule * pApp;


#define SETSTYLE(fn,fs,cs,fg,bg)		{ sizeof(STYLE), fn, fs, cs, fg, bg }
#define SETFG(x)					{ sizeof(STYLE), _T(""), 0, def_charset, x }
#define SETBG(x)					{ sizeof(STYLE), _T(""), 0, def_charset, 0, x }
#define SETFGBG(fg,bg)				{ sizeof(STYLE), _T(""), 0, def_charset, fg, bg }
#define SETSIZE(x)					{ sizeof(STYLE), _T(""), x, def_charset }
#define SETSIZEFG(s,fg)				{ sizeof(STYLE), _T(""), s, def_charset, fg }
#define SETEOL(fg,bg)				{ sizeof(STYLE), _T(""), 0, def_charset, fg, bg, false, false, false, true }

#define BLACK			0xFF000000 //0x000000 == not set
#define WHITE			0xFFFFFF
#define	RED				0x0000FF
#define	GREEN			0x00FF00
#define	BLUE			0xFF0000
#define DARKGREEN		0x008000
#define GRAY			0x808080
#define SILVER			0xC0C0C0
#define DARKBLUE		0x800000
#define STEELBLUE		0xA56E3A
#define PURPLE			0x800080
#define DARKORANGE		0x0080FF
#define MIDNIGHTBLUE	0x6A240A
#define MEDIUMBLUE		0xCC3300
#define MAROON			0x000080
#define AQUA			0x80FFFF
#define FIREBRICK		0x0033CC

#define NULLSTYLE		{ sizeof(STYLE), _T("") }
#define DEFAULTSTYLE	{ sizeof(STYLE), _T("Fixedsys"), 10 , def_charset }
#define EOSTYLE			{ -1, /*_T(""),*/ NULLSTYLE }
#define COMMENTSTYLE	{ sizeof(STYLE), _T("Comic Sans MS"), 10 , 0, DARKGREEN }
#define STRINGSTYLE		SETFG(GRAY)
#define CPPKEYWORD		SETFG(BLUE)
#define NCLOSESTRING	SETEOL(GRAY, AQUA)

BYTE def_charset = CharsetFromCodepage(::GetACP());

KEYWORDLIST KeyWords_NULL = { _T(""), _T(""), _T(""), _T(""), _T(""), _T(""), _T(""), _T(""), _T("") };

EDITLEXER lexDefault = { SCLEX_NULL, IDS_LEXDEFAULT, /*"文本文档",*/ _T("txt;wtx;log;asc"), _T(""), II_TXT, &KeyWords_NULL, {
		/*  0 */ { STYLE_DEFAULT, /*"默认样式",*/ DEFAULTSTYLE },
		/*  1 */ { STYLE_LINENUMBER, /*"页边空白和行号",*/ SETSIZE(0xFF000002) }, //size = -2
		/*  2 */ { STYLE_BRACELIGHT, /*"匹配的括号",*/ SETSIZEFG(0x7F000001, RED) }, //size = +1
		/*  3 */ { STYLE_BRACEBAD, /*"匹配的错误括号",*/ SETSIZEFG(0x7F000001, DARKBLUE) }, //size = +1
		/*  4 */ { STYLE_CONTROLCHAR, /*"控制符 (未使用的)"*/ },
		/*  5 */ { STYLE_INDENTGUIDE, /*"缩排向导 (仅颜色)"*/ },
		/*  6 */ { SCI_SETSELFORE+SCI_SETSELBACK, /*"选择的文本 (仅颜色)",*/ SETFGBG(WHITE, 0xFF8000) },
		/*  7 */ { SCI_SETWHITESPACEFORE+SCI_SETWHITESPACEBACK, /*"空白处 (仅颜色)",*/ SETFG(RED) },
		/*  8 */ { SCI_SETCARETLINEBACK, /*"当前行背景 (仅颜色)"*/ },
		/*  9 */ { SCI_SETCARETFORE, /*"^ 符号前景 (仅颜色)"*/ },
		/* 10 */ { SCI_SETCARETWIDTH, /*"C^ 符号宽度 (大小从 1 到 3)"*/ },
		/* 11 */ { SCI_SETEDGECOLOUR, /*"排版线 (仅颜色)"*/ },
		/* 12 */ { SCI_MARKERDEFINEPIXMAP, /*"书签图标 (1 到 9)",*/ SETSIZE(1) },
		/* 13 */ { SCI_MARKERSETFORE+SCI_MARKERSETBACK, /*"书签行背景 (仅颜色)",*/ SETBG(0xEFCDAB) },
		EOSTYLE } };

EDITLEXER lexSub = { SCLEX_NULL, IDS_LEXSUB, /*"文本字幕",*/ _T("srt;idx"), _T(""), II_TXT, &KeyWords_NULL, {
		{ STYLE_DEFAULT, /*"默认样式",*/ NULLSTYLE },
		EOSTYLE } };


EDITLEXER lexMSDOS = { SCLEX_ASCII, IDS_LEXMSDOS, /*"MS-DOS",*/ _T("nfo;diz"), _T(""), II_TXT, &KeyWords_NULL, {
		{ STYLE_DEFAULT, /*"默认样式",*/ SETSTYLE(_T("MS LineDraw"), 10, 0x02, SILVER, BLACK) },
		EOSTYLE } };


KEYWORDLIST KeyWords_HTML = {
            _T("a abbr acronym address applet area b base basefont bdo big blockquote body bordercolor br button caption ")
            _T("center cite code col colgroup dd del dfn dir div dl dt em fieldset font form frame frameset h1 h2 ")
            _T("h3 h4 h5 h6 head hr html i iframe img input ins isindex kbd label legend li link map menu meta ")
            _T("noframes noscript object ol optgroup option p param pre q s samp script select small span strike ")
            _T("strong style sub sup table tbody td textarea tfoot th thead title tr tt u ul var xml xmlns abbr ")
            _T("accept-charset accept accesskey action align alink alt archive axis background bgcolor border ")
            _T("cellpadding cellspacing char charoff charset checked cite class classid clear codebase codetype ")
            _T("color cols colspan compact content coords data datafld dataformatas datapagesize datasrc ")
            _T("datetime declare defer dir disabled enctype event face for frame frameborder headers height href ")
            _T("hreflang hspace http-equiv id ismap label lang language leftmargin link longdesc marginwidth ")
            _T("marginheight maxlength media method multiple name nohref noresize noshade nowrap object ")
            _T("onblur onchange onclick ondblclick onfocus onkeydown onkeypress onkeyup onload onmousedown ")
            _T("onmousemove onmouseover onmouseout onmouseup onreset onselect onsubmit onunload profile ")
            _T("prompt readonly rel rev rows rowspan rules scheme scope scrolling selected shape size span src standby ")
            _T("start style summary tabindex target text title topmargin type usemap valign value valuetype ")
            _T("version vlink vspace width text password checkbox radio submit reset file hidden image public ")
            _T("!doctype"),
            _T("abstract boolean break byte case catch char class const continue debugger default delete do ")
            _T("double else enum export extends final finally float for function goto if implements import in ")
            _T("instanceof int interface long native new package private protected public return short static ")
            _T("super switch synchronized this throw throws transient try typeof var void volatile while with"),
            _T("and begin case call continue do each else elseif end erase error event exit false for function get ")
            _T("gosub goto if implement in load loop lset me mid new next not nothing on or property raiseevent ")
            _T("rem resume return rset select set stop sub then to true unload until wend while with withevents ")
            _T("attribute alias as boolean byref byte byval const compare currency date declare dim double enum ")
            _T("explicit friend global integer let lib long module object option optional preserve private property ")
            _T("public redim single static string type variant"),
            _T("and assert break class continue def del elif else except exec finally for from global if import in is ")
            _T("lambda None not or pass print raise return try while yield"),
            _T("and argv as argc break case cfunction class const continue declare default define do die echo else elseif ")
            _T("empty enddeclare endfor endforeach endif endswitch endwhile e_all e_parse e_error e_warning eval exit ")
            _T("extends false for foreach function global http_cookie_vars http_get_vars http_post_vars http_post_files ")
            _T("http_env_vars http_server_vars if include include_once isset list new not null old_function or parent ")
            _T("php_os php_self php_version print require require_once return static switch stdclass this true unset use ")
            _T("var xor virtual while __file__ __line__ __sleep __wakeup"),
            _T("ELEMENT DOCTYPE ATTLIST ENTITY NOTATION"),
            _T(""), _T(""), _T("") };


EDITLEXER lexHTML = { SCLEX_HTML, IDS_LEXHTML, /*"网页源代码",*/ _T("html;htm;asp;aspx;shtml;htd;xhtml;php;php3;phtml;htt;cfm;tpl;hta"), _T(""), II_NONE, &KeyWords_HTML, {
                          { STYLE_DEFAULT, /*"默认",*/ DEFAULTSTYLE },
                          //{ SCE_H_DEFAULT, "默认" },
                          { SCE_H_TAG, /*"HTML 标签",*/ SETFG(DARKBLUE) },
                          { SCE_H_TAGUNKNOWN, /*"HTML 未知标签",*/ SETFG(DARKBLUE) },
                          { SCE_H_ATTRIBUTE, /*"HTML 属性",*/ SETFG(RED) },
                          { SCE_H_ATTRIBUTEUNKNOWN, /*"HTML 未知属性",*/ SETFG(RED) },
                          { SCE_H_VALUE, /*"HTML 值",*/ SETFG(STEELBLUE) },
                          { SCE_H_DOUBLESTRING, /*"双引号引用的 HTML 字串",*/ SETFG(STEELBLUE) },
                          { SCE_H_SINGLESTRING, /*"单引号引用的 HTML 字串",*/ SETFG(STEELBLUE) },
                          { SCE_H_OTHER, /*"HTML 其他内置标签",*/ SETFG(STEELBLUE) },
                          { SCE_H_COMMENT, /*"HTML 注释",*/ STRINGSTYLE },
                          { SCE_H_ENTITY, /*"HTML 实体",*/ SETFG(PURPLE) },
                          { SCE_H_TAGEND, /*"XML 关闭标签",*/ SETFG(DARKBLUE) },
                          { SCE_H_XMLSTART, /*"XML 标识符开始",*/ SETFG(RED) },
                          { SCE_H_XMLEND, /*"XML 标识符结束",*/ SETFG(RED) },
                          { SCE_H_ASP, /*"ASP 开始标签",*/ SETFG(DARKBLUE) },
                          { SCE_H_ASPAT, /*"ASP 开始标签 @",*/ SETFG(DARKBLUE) },
                          //{ SCE_H_SCRIPT, "Script" },
                          { SCE_H_CDATA, /*"CDATA"*/ },
                          //{ SCE_H_XCCOMMENT, "XC Comment" },
                          { SCE_H_QUESTION, /*"PHP 开始标签",*/ SETFG(DARKBLUE) },
                          { SCE_HPHP_DEFAULT, /*"PHP 默认"*/ },
                          { SCE_HPHP_HSTRING, /*"PHP 字串",*/ COMMENTSTYLE },
                          { SCE_HPHP_SIMPLESTRING, /*"PHP 简单字串",*/ COMMENTSTYLE },
                          { SCE_HPHP_WORD, /*"PHP 关键字",*/ SETFG(PURPLE) },
                          { SCE_HPHP_NUMBER, /*"PHP 数字",*/ SETFG(RED) },
                          { SCE_HPHP_VARIABLE, /*"PHP 变量",*/ SETFG(DARKBLUE) },
                          { SCE_HPHP_HSTRING_VARIABLE, /*"PHP 字串变量",*/ SETFG(DARKBLUE) },
                          { SCE_HPHP_COMPLEX_VARIABLE, /*"PHP 复变量",*/ SETFG(DARKBLUE) },
                          { SCE_HPHP_COMMENT, /*"PHP 注释",*/ SETFG(DARKORANGE) },
                          { SCE_HPHP_COMMENTLINE, /*"PHP 注释行",*/ SETFG(DARKORANGE) },
                          { SCE_HPHP_OPERATOR, /*"PHP 运算符"*/ },
                          //{ SCE_HJ_START, "JS Start" },
                          { SCE_HJ_DEFAULT, /*"JS 默认"*/ },
                          { SCE_HJ_COMMENT, /*"JS 注释",*/ COMMENTSTYLE },
                          { SCE_HJ_COMMENTLINE, /*"JS 注释行",*/ COMMENTSTYLE },
                          { SCE_HJ_COMMENTDOC, /*"JS 注释文档",*/ COMMENTSTYLE },
                          { SCE_HJ_NUMBER, /*"JS 数字",*/ SETFG(RED) },
                          { SCE_HJ_WORD, /*"JS 标识符"*/ },
                          { SCE_HJ_KEYWORD, /*"JS 关键字",*/ SETFG(MIDNIGHTBLUE) },
                          { SCE_HJ_DOUBLESTRING, /*"JS 双字串",*/ COMMENTSTYLE },
                          { SCE_HJ_SINGLESTRING, /*"JS 单字串",*/ COMMENTSTYLE },
                          { SCE_HJ_STRINGEOL, /*"JS 字串结束行",*/ COMMENTSTYLE },
                          { SCE_HJ_SYMBOLS, /*"JS 符号"*/ },
                          { SCE_HJ_REGEX, /*"JS Regex"*/ },
                          //{ SCE_HJA_START, "ASP JS Start" },
                          { SCE_HJA_DEFAULT, /*"ASP JS 默认"*/ },
                          { SCE_HJA_COMMENT, /*"ASP JS 注释",*/ COMMENTSTYLE },
                          { SCE_HJA_COMMENTLINE, /*"ASP JS 注释行",*/ COMMENTSTYLE },
                          { SCE_HJA_COMMENTDOC, /*"ASP JS 注释文档",*/ COMMENTSTYLE },
                          { SCE_HJA_NUMBER, /*"ASP JS 数字",*/ SETFG(RED) },
                          { SCE_HJA_WORD, /*"ASP JS 标识符"*/ },
                          { SCE_HJA_KEYWORD, /*"ASP JS 关键字",*/ SETFG(MIDNIGHTBLUE) },
                          { SCE_HJA_DOUBLESTRING, /*"ASP JS 双字串",*/ COMMENTSTYLE },
                          { SCE_HJA_SINGLESTRING, /*"ASP JS 单字串",*/ COMMENTSTYLE },
                          { SCE_HJA_STRINGEOL, /*"ASP JS 字串结束行",*/ COMMENTSTYLE },
                          { SCE_HJA_SYMBOLS, /*"ASP JS 符号"*/ },
                          { SCE_HJA_REGEX, /*"ASP JS Regex"*/ },
                          //{ SCE_HB_START, "VBS Start" },
                          { SCE_HB_DEFAULT, /*"VBS 默认"*/ },
                          { SCE_HB_COMMENTLINE, /*"VBS 注释行",*/ COMMENTSTYLE },
                          { SCE_HB_NUMBER, /*"VBS 数字",*/ SETFG(RED) },
                          { SCE_HB_WORD, /*"VBS 关键字",*/ SETFG(PURPLE) },
                          { SCE_HB_STRING, /*"VBS 字串",*/ COMMENTSTYLE },
                          { SCE_HB_STRINGEOL, /*"VBS 字串结束行",*/ COMMENTSTYLE },
                          { SCE_HB_IDENTIFIER, /*"VBS 标识符"*/ },
                          //{ SCE_HBA_START, "ASP VBS Start" },
                          { SCE_HBA_DEFAULT, /*"ASP VBS 默认"*/ },
                          { SCE_HBA_COMMENTLINE, /*"ASP VBS 注释行",*/ COMMENTSTYLE },
                          { SCE_HBA_NUMBER, /*"ASP VBS 数字"*/ },
                          { SCE_HBA_WORD, /*"ASP VBS 关键字",*/ SETFG(PURPLE) },
                          { SCE_HBA_STRING, /*"ASP VBS 字串",*/ COMMENTSTYLE },
                          { SCE_HBA_STRINGEOL, /*"ASP VBS 字串结束行",*/ COMMENTSTYLE },
                          { SCE_HBA_IDENTIFIER, /*"ASP VBS 标识符"*/ },
                          EOSTYLE } };


KEYWORDLIST KeyWords_XML = { _T(""), _T(""), _T(""), _T(""), _T(""),
			_T("ELEMENT DOCTYPE ATTLIST ENTITY NOTATION"), _T(""), _T(""), _T("") };


EDITLEXER lexXML = { SCLEX_XML, IDS_LEXXML, /*"XML 文档",*/ _T("xml;xsl;svg;xul;xsd;dtd;xslt;axl;rdf;manifest"), _T(""), II_NONE, &KeyWords_XML, {
                         { STYLE_DEFAULT, /*"默认",*/ DEFAULTSTYLE },
                         { SCE_H_TAG, /*"XML 标签",*/ SETFG(PURPLE) },
                         { SCE_H_TAGEND, /*"XML 关闭标签",*/ SETFG(PURPLE) },
                         { SCE_H_ATTRIBUTE, /*"XML 属性",*/ SETFG(RED) },
                         { SCE_H_VALUE, /*"XML 值",*/ SETFG(STEELBLUE) },
                         { SCE_H_DOUBLESTRING, /*"XML 双引字串",*/ SETFG(STEELBLUE) },
                         { SCE_H_SINGLESTRING, /*"XML 单引字串",*/ SETFG(STEELBLUE) },
                         { SCE_H_OTHER, /*"XML 其它内置标签",*/ SETFG(STEELBLUE) },
                         { SCE_H_COMMENT, /*"XML 注释",*/ STRINGSTYLE },
                         { SCE_H_ENTITY, /*"XML 实体",*/ SETFG(PURPLE) },
                         { SCE_H_XMLSTART, /*"XML 标识符开始",*/ SETFG(RED) },
                         { SCE_H_XMLEND, /*"XML 标识符结束",*/ SETFG(RED) },
                        { SCE_H_CDATA, /*"CDATA",*/ COMMENTSTYLE },
                         EOSTYLE } };


KEYWORDLIST KeyWords_CSS = {
           _T("azimuth background background-attachment background-color ")
           _T("background-image background-position background-repeat border ")
           _T("border-bottom border-bottom-color border-bottom-style ")
           _T("border-bottom-width border-collapse border-color border-left ")
           _T("border-left-color border-left-style border-left-width ")
           _T("border-right border-right-color border-right-style ")
           _T("border-right-width border-spacing border-style border-top ")
           _T("border-top-color border-top-style border-top-width border-width ")
           _T("bottom caption-side clear clip color content counter-increment ")
           _T("counter-reset cue cue-after cue-before cursor direction display ")
           _T("elevation empty-cells float font font-family font-size ")
           _T("font-size-adjust font-stretch font-style font-variant ")
           _T("font-weight height left letter-spacing line-height list-style ")
           _T("list-style-image list-style-position list-style-type margin ")
           _T("margin-bottom margin-left margin-right margin-top marker-offset ")
           _T("marks max-height max-width min-height min-width orphans outline ")
           _T("outline-color outline-style outline-width overflow padding ")
           _T("padding-bottom padding-left padding-right padding-top page ")
           _T("page-break-after page-break-before page-break-inside pause ")
           _T("pause-after pause-before pitch pitch-range play-during position ")
           _T("quotes richness right scrollbar-3dlight-color ")
           _T("scrollbar-arrow-color scrollbar-base-color ")
           _T("scrollbar-darkshadow-color scrollbar-face-color ")
           _T("scrollbar-highlight-color scrollbar-shadow-color ")
           _T("scrollbar-track-color size speak speak-header speak-numeral ")
           _T("speak-punctuation speech-rate stress table-layout text-align ")
           _T("text-decoration text-indent text-shadow text-transform top ")
           _T("unicode-bidi vertical-align visibility voice-family volume ")
           _T("white-space widows width word-spacing z-index"),
           _T("first-letter first-line active link visited hover"),
           _T(""), _T(""), _T(""), _T(""), _T(""), _T(""), _T("") };


EDITLEXER lexCSS = { SCLEX_CSS, IDS_LEXCSS, /*"CSS 样式表",*/ _T("css"), _T(""), II_CSS, &KeyWords_CSS, {
                         { STYLE_DEFAULT, /*"默认",*/ DEFAULTSTYLE },
                         //{ SCE_CSS_DEFAULT, "默认", NULLSTYLE },
                         { SCE_CSS_TAG, /*"标签",*/ SETFG(MIDNIGHTBLUE) },
                         { SCE_CSS_CLASS, /*"类",*/ SETFG(RED) },
                         { SCE_CSS_PSEUDOCLASS, /*"伪类",*/ SETFG(RED) },
                         { SCE_CSS_UNKNOWN_PSEUDOCLASS, /*"未知的伪类",*/ SETFG(RED) },
                         { SCE_CSS_OPERATOR, /*"运算符",*/ NULLSTYLE },
                         { SCE_CSS_IDENTIFIER, /*"标识符",*/ SETFG(STEELBLUE) },
                         { SCE_CSS_UNKNOWN_IDENTIFIER, /*"未知的标识符",*/ NULLSTYLE },
                         { SCE_CSS_VALUE, /*"值",*/ NULLSTYLE },
                         { SCE_CSS_COMMENT, /*"注释",*/ COMMENTSTYLE },
                         { SCE_CSS_ID, /*"Id",*/ NULLSTYLE },
                         { SCE_CSS_IMPORTANT, /*"重要的",*/ NULLSTYLE },
                         { SCE_CSS_DIRECTIVE, /*"指示的",*/ NULLSTYLE },
                         { SCE_CSS_DOUBLESTRING, /*"双引号引用的字串",*/ COMMENTSTYLE },
                         { SCE_CSS_SINGLESTRING, /*"单引号引用的字串",*/ COMMENTSTYLE },
                         EOSTYLE } };


KEYWORDLIST KeyWords_CPP = {
           _T("and and_eq asm auto bitand bitor bool break case catch char class ")
           _T("compl const const_cast continue default delete do double ")
           _T("dynamic_cast else enum explicit export extern false float for friend ")
           _T("goto if inline int long mutable namespace new not not_eq operator or ")
           _T("or_eq private protected public register reinterpret_cast return short ")
           _T("signed sizeof static static_cast struct switch template this throw true ")
           _T("try typedef typeid typename union unsigned using virtual void volatile ")
           _T("wchar_t while xor xor_eq"),
           _T(""),
           _T("a addindex addtogroup anchor arg attention author b brief bug c class code ")
           _T("date def defgroup deprecated dontinclude e em endcode endhtmlonly endif ")
           _T("endlatexonly endlink endverbatim enum example exception f$ f[ f] file fn ")
           _T("hideinitializer htmlinclude htmlonly if image include ingroup internal invariant ")
           _T("interface latexonly li line link mainpage name namespace nosubgrouping note ")
           _T("overload p page par param post pre ref relates remarks return retval sa ")
           _T("section see showinitializer since skip skipline struct subsection test throw ")
           _T("todo typedef union until var verbatim verbinclude version warning ")
           _T("weakgroup $ @ \\ & < > # { }"),
           _T(""), _T(""), _T(""), _T(""), _T(""), _T("") };


EDITLEXER lexCPP = { SCLEX_CPP, IDS_LEXCPP, /*"C/C++ 源代码",*/ _T("cpp;c;cxx;def;h;hpp;hxx;hm;inl;inc"), _T(""), II_CPP, &KeyWords_CPP, {
                         { STYLE_DEFAULT, /*"默认",*/ DEFAULTSTYLE },
                         { SCE_C_COMMENT, /*"注释",*/ COMMENTSTYLE },
                         { SCE_C_COMMENTLINE, /*"注释行",*/ COMMENTSTYLE },
                         { SCE_C_COMMENTDOC, /*"注释文档",*/ COMMENTSTYLE },
                         { SCE_C_COMMENTLINEDOC, /*"注释行文档",*/ COMMENTSTYLE },
                         { SCE_C_WORD, /*"关键字",*/ CPPKEYWORD },
                         { SCE_C_IDENTIFIER, /*"标识符",*/ NULLSTYLE },
                         { SCE_C_NUMBER, /*"数字",*/ SETFG(PURPLE) },
                         { SCE_C_STRING, /*"字串",*/ STRINGSTYLE },
                         { SCE_C_STRINGEOL, /*"未关闭的字串",*/ NCLOSESTRING },
                         { SCE_C_CHARACTER, /*"字符",*/ STRINGSTYLE },
                         { SCE_C_OPERATOR, /*"运算符",*/ SETFG(DARKBLUE) },
                         { SCE_C_PREPROCESSOR, /*"预处理器",*/ SETFG(BLUE) },
                         { SCE_C_COMMENTDOCKEYWORD, /*"注释文档关键字",*/ COMMENTSTYLE },
                         { SCE_C_COMMENTDOCKEYWORDERROR, /*"注释文档关键字错误",*/ COMMENTSTYLE },
                         EOSTYLE } };


//EDITLEXER lexH = { SCLEX_CPP, IDS_LEXH, /*"C/C++ 头文件",*/ _T("h;hpp;hxx;hm;inl;inc"), _T(""), II_H, &KeyWords_CPP, {
//                         { STYLE_DEFAULT, /*"默认",*/ DEFAULTSTYLE },
//                         { SCE_C_COMMENT, /*"注释",*/ COMMENTSTYLE },
//                         { SCE_C_COMMENTLINE, /*"注释行",*/ COMMENTSTYLE },
//                         { SCE_C_COMMENTDOC, /*"注释文档",*/ COMMENTSTYLE },
//                         { SCE_C_COMMENTLINEDOC, /*"注释行文档",*/ COMMENTSTYLE },
//                         { SCE_C_WORD, /*"关键字",*/ CPPKEYWORD },
//                         { SCE_C_IDENTIFIER, /*"标识符",*/ NULLSTYLE },
//                         { SCE_C_NUMBER, /*"数字",*/ SETFG(PURPLE) },
//                         { SCE_C_STRING, /*"字串",*/ STRINGSTYLE },
//                         { SCE_C_STRINGEOL, /*"未关闭的字串",*/ NCLOSESTRING },
//                         { SCE_C_CHARACTER, /*"字符",*/ STRINGSTYLE },
//                         { SCE_C_OPERATOR, /*"运算符",*/ SETFG(DARKBLUE) },
//                         { SCE_C_PREPROCESSOR, /*"预处理器",*/ SETFG(BLUE) },
//                         { SCE_C_COMMENTDOCKEYWORD, /*"注释文档关键字",*/ COMMENTSTYLE },
//                         { SCE_C_COMMENTDOCKEYWORDERROR, /*"注释文档关键字错误",*/ COMMENTSTYLE },
//                         EOSTYLE } };


KEYWORDLIST KeyWords_CS = {
          _T("abstract as base bool break byte case catch char checked class const ")
          _T("continue decimal default delegate do double else enum event explicit ")
          _T("extern false finally fixed float for foreach goto if implicit in int interface ")
          _T("internal is lock long namespace new null object operator out override ")
          _T("params private protected public readonly ref return sbyte sealed short ")
          _T("sizeof stackalloc static string struct switch this throw true try typeof ")
          _T("uint ulong unchecked unsafe ushort using virtual void while"),
          _T(""),
          _T("a addindex addtogroup anchor arg attention author b brief bug c class code ")
          _T("date def defgroup deprecated dontinclude e em endcode endhtmlonly endif ")
          _T("endlatexonly endlink endverbatim enum example exception f$ f[ f] file fn ")
          _T("hideinitializer htmlinclude htmlonly if image include ingroup internal invariant ")
          _T("interface latexonly li line link mainpage name namespace nosubgrouping note ")
          _T("overload p page par param post pre ref relates remarks return retval sa ")
          _T("section see showinitializer since skip skipline struct subsection test throw todo ")
          _T("typedef union until var verbatim verbinclude version warning weakgroup $ @ ")
          _T("\\ & < > # { }"),
          _T(""), _T(""), _T(""), _T(""), _T(""), _T("") };


EDITLEXER lexCS = { SCLEX_CPP, IDS_LEXCS, /*"C# 源代码",*/ _T("cs"), _T(""), II_CS, &KeyWords_CS, {
			{ STYLE_DEFAULT, /*"默认",*/ DEFAULTSTYLE },
			//{ SCE_C_DEFAULT, "默认", NULLSTYLE },
			{ SCE_C_COMMENT, /*"注释",*/ COMMENTSTYLE },
			{ SCE_C_COMMENTLINE, /*"注释行",*/ COMMENTSTYLE },
			{ SCE_C_COMMENTDOC, /*"注释文档",*/ COMMENTSTYLE },
			{ SCE_C_COMMENTLINEDOC, /*"注释行文档",*/ COMMENTSTYLE },
			{ SCE_C_WORD, /*"关键字",*/ CPPKEYWORD },
			{ SCE_C_IDENTIFIER, /*"标识符",*/ NULLSTYLE },
			{ SCE_C_NUMBER, /*"数字",*/ SETFG(PURPLE) },
			{ SCE_C_STRING, /*"字串",*/ STRINGSTYLE },
			{ SCE_C_VERBATIM, /*"Verbatim字串",*/ COMMENTSTYLE },
			{ SCE_C_STRINGEOL, /*"未关闭的字串",*/ NCLOSESTRING },
			{ SCE_C_CHARACTER, /*"字符",*/ COMMENTSTYLE },
			{ SCE_C_OPERATOR, /*"运算符",*/ SETFG(MIDNIGHTBLUE) },
			{ SCE_C_PREPROCESSOR, /*"预处理器",*/ CPPKEYWORD },
			{ SCE_C_COMMENTDOCKEYWORD, /*"注释文档关键字",*/ COMMENTSTYLE },
			{ SCE_C_COMMENTDOCKEYWORDERROR, /*"注释文档关键字错误",*/ COMMENTSTYLE },
			EOSTYLE } };


KEYWORDLIST KeyWords_RC = {
          _T("ACCELERATORS ALT AUTO3STATE AUTOCHECKBOX AUTORADIOBUTTON ")
          _T("BEGIN BITMAP BLOCK BUTTON CAPTION CHARACTERISTICS CHECKBOX ")
          _T("CLASS COMBOBOX CONTROL CTEXT CURSOR DEFPUSHBUTTON DIALOG ")
          _T("DIALOGEX DISCARDABLE EDITTEXT END EXSTYLE FONT GROUPBOX ")
          _T("ICON LANGUAGE LISTBOX LTEXT MENU MENUEX MENUITEM ")
          _T("MESSAGETABLE POPUP PUSHBUTTON RADIOBUTTON RCDATA RTEXT ")
          _T("SCROLLBAR SEPARATOR SHIFT STATE3 STRINGTABLE STYLE ")
          _T("TEXTINCLUDE VALUE VERSION VERSIONINFO VIRTKEY"),
          _T(""), _T(""), _T(""), _T(""), _T(""), _T(""), _T(""), _T("") };


EDITLEXER lexRC = { SCLEX_CPP, IDS_LEXRC, /*"资源脚本",*/ _T("rc;rc2;rct;dlg"), _T(""), II_RC, &KeyWords_RC, {
                        { STYLE_DEFAULT, /*"默认",*/ DEFAULTSTYLE },
                        //{ SCE_C_DEFAULT, "默认", NULLSTYLE },
                        { SCE_C_COMMENT, /*"注释",*/ COMMENTSTYLE },
                        { SCE_C_COMMENTLINE, /*"注释行",*/ COMMENTSTYLE },
                        { SCE_C_COMMENTDOC, /*"注释文档",*/ COMMENTSTYLE },
                        { SCE_C_WORD, /*"关键字",*/ SETFG(MIDNIGHTBLUE) },
                        { SCE_C_IDENTIFIER, /*"标识符",*/ NULLSTYLE },
                        { SCE_C_NUMBER, /*"数字",*/ SETFG(PURPLE) },
                        { SCE_C_STRING, /*"字串",*/ STRINGSTYLE },
                        { SCE_C_STRINGEOL, /*"未关闭的字串",*/ NCLOSESTRING },
                        { SCE_C_CHARACTER, /*"字符",*/ STRINGSTYLE },
                        { SCE_C_OPERATOR, /*"运算符",*/ SETFG(MIDNIGHTBLUE) },
                        { SCE_C_PREPROCESSOR, /*"预处理器",*/ SETFG(BLUE) },
                        EOSTYLE } };


KEYWORDLIST KeyWords_MAK = {
                               _T(""), _T(""), _T(""), _T(""), _T(""), _T(""), _T(""), _T(""), _T("") };


EDITLEXER lexMAK = { SCLEX_MAKEFILE, IDS_LEXMAK, /*"Makefiles",*/ _T("mak;make;mk"), _T(""), II_MAK, &KeyWords_MAK, {
                         { STYLE_DEFAULT, /*"默认",*/ DEFAULTSTYLE },
                         //{ SCE_MAKE_DEFAULT, "默认", NULLSTYLE },
                         { SCE_MAKE_COMMENT, /*"注释",*/ COMMENTSTYLE },
                         { SCE_MAKE_PREPROCESSOR, /*"预处理器",*/ SETFG(DARKORANGE) },
                         { SCE_MAKE_IDENTIFIER, /*"标识符",*/ NULLSTYLE },
                         { SCE_MAKE_OPERATOR, /*"运算符",*/ NULLSTYLE },
                         { SCE_MAKE_TARGET, /*"目标",*/ NULLSTYLE },
                         { SCE_MAKE_IDEOL, /*"ID EOL",*/ NULLSTYLE },
                         EOSTYLE } };


KEYWORDLIST KeyWords_VBS = {
           _T("and begin case call continue do each else elseif end erase error event exit false ")
           _T("for function get gosub goto if implement in load loop lset me mid new next not nothing ")
           _T("on or property raiseevent rem resume return rset select set stop sub then to true unload ")
           _T("until wend while with withevents attribute alias as boolean byref byte byval const compare ")
           _T("currency date declare dim double enum explicit friend global integer let lib long module object ")
           _T("option optional preserve private property public redim single static string type variant"),
           _T(""), _T(""), _T(""), _T(""), _T(""), _T(""), _T(""), _T("") };


EDITLEXER lexVBS = { SCLEX_VBSCRIPT, IDS_LEXVBS, /*"VB 脚本",*/ _T("vbs;dsm"), _T(""), II_NONE, &KeyWords_VBS, {
                         { STYLE_DEFAULT, /*"默认",*/ DEFAULTSTYLE },
                         //{ SCE_B_DEFAULT, "默认", NULLSTYLE },
                         { SCE_B_COMMENT, /*"注释",*/ COMMENTSTYLE },
                         { SCE_B_NUMBER, /*"数字",*/ SETFG(PURPLE) },
                         { SCE_B_KEYWORD, /*"关键字",*/ CPPKEYWORD },
                         { SCE_B_STRING, /*"字串",*/ STRINGSTYLE },
                         { SCE_B_STRINGEOL, /*"未关闭的字串",*/ NCLOSESTRING },
                         { SCE_B_OPERATOR, /*"运算符",*/ NULLSTYLE },
                         { SCE_B_IDENTIFIER, /*"标识符",*/ NULLSTYLE },
                         //{ SCE_B_DATE, "Date", NULLSTYLE },
                         //{ SCE_B_KEYWORD2, "Keyword 2", NULLSTYLE },
                         //{ SCE_B_KEYWORD3, "Keyword 3", NULLSTYLE },
                         //{ SCE_B_KEYWORD4, "Keyword 4", NULLSTYLE },
                         { SCE_B_CONSTANT, /*"常量",*/ NULLSTYLE },
                         { SCE_B_PREPROCESSOR, /*"预处理器",*/ SETFG(MIDNIGHTBLUE) },
                         { SCE_B_ASM, /*"汇编",*/ SETFG(DARKORANGE) },
                         EOSTYLE } };


KEYWORDLIST KeyWords_VB = {
          _T("addhandler addressof andalso alias and ansi as assembly auto boolean byref ")
          _T("byte byval call case catch cbool cbyte cchar cdate cdec cdbl char cint class ")
          _T("clng cobj const cshort csng cstr ctype date decimal declare default delegate ")
          _T("dim do double each else elseif end enum erase error event exit false finally for ")
          _T("friend function get gettype goto  handles if implements imports in inherits integer ")
          _T("interface is let lib like long loop me mod module mustinherit mustoverride ")
          _T("mybase myclass namespace new next not nothing notinheritable notoverridable ")
          _T("object on option optional or orelse overloads overridable overrides paramarray ")
          _T("preserve private property protected public raiseevent readonly redim rem ")
          _T("removehandler resume return select set shadows shared short single static step ")
          _T("stop string structure sub synclock then throw to true try typeof unicode until ")
          _T("variant when while with withevents writeonly xor"),
          _T(""), _T(""), _T(""), _T(""), _T(""), _T(""), _T(""), _T("") };


EDITLEXER lexVB = { SCLEX_VB, IDS_LEXVB, /*"Visual Basic",*/ _T("vb;bas;frm;cls;ctl;pag;dsr;dob"), _T(""), II_VB, &KeyWords_VB, {
                        { STYLE_DEFAULT, /*"默认",*/ DEFAULTSTYLE },
                        //{ SCE_B_DEFAULT, "默认", NULLSTYLE },
                        { SCE_B_COMMENT, /*"注释",*/ COMMENTSTYLE },
                        { SCE_B_NUMBER, /*"数字",*/ SETFG(PURPLE) },
                        { SCE_B_KEYWORD, /*"关键字",*/ CPPKEYWORD },
                        { SCE_B_STRING, /*"字串",*/ STRINGSTYLE },
                        { SCE_B_STRINGEOL, /*"未关闭的字串",*/ NCLOSESTRING },
                        { SCE_B_OPERATOR, /*"运算符",*/ NULLSTYLE },
                        { SCE_B_IDENTIFIER, /*"标识符",*/ NULLSTYLE },
                        //{ SCE_B_DATE, "Date", NULLSTYLE },
                        //{ SCE_B_KEYWORD2, "Keyword 2", NULLSTYLE },
                        //{ SCE_B_KEYWORD3, "Keyword 3", NULLSTYLE },
                        //{ SCE_B_KEYWORD4, "Keyword 4", NULLSTYLE },
                        { SCE_B_CONSTANT, /*"常量",*/ NULLSTYLE },
                        { SCE_B_PREPROCESSOR, /*"预处理器",*/ SETFG(MIDNIGHTBLUE) },
                        { SCE_B_ASM, /*"汇编",*/ SETFG(DARKORANGE) },
                        EOSTYLE } };


KEYWORDLIST KeyWords_JS = {
          _T("abstract boolean break byte case catch char class const continue ")
          _T("debugger default delete do double else enum export extends final ")
          _T("finally float for function goto if implements import in instanceof int ")
          _T("interface long native new package private protected public return ")
          _T("short static super switch synchronized this throw throws transient try ")
          _T("typeof var void volatile while with"),
          _T(""), _T(""), _T(""), _T(""), _T(""), _T(""), _T(""), _T("") };


EDITLEXER lexJS = { SCLEX_CPP, IDS_LEXJS, /*"Java 脚本",*/ _T("js;jse"), _T(""), II_NONE, &KeyWords_JS, {
                        { STYLE_DEFAULT, /*"默认",*/ DEFAULTSTYLE },
                        //{ SCE_C_DEFAULT, "默认", NULLSTYLE },
                        { SCE_C_COMMENT, /*"注释",*/ COMMENTSTYLE },
                        { SCE_C_COMMENTLINE, /*"注释行",*/ COMMENTSTYLE },
                        { SCE_C_COMMENTDOC, /*"注释文档",*/ COMMENTSTYLE },
                        { SCE_C_COMMENTLINEDOC, /*"注释行文档",*/ COMMENTSTYLE },
                        { SCE_C_WORD, /*"关键字",*/ CPPKEYWORD },
                        { SCE_C_IDENTIFIER, /*"标识符",*/ NULLSTYLE },
                        { SCE_C_NUMBER, /*"数字",*/ SETFG(PURPLE) },
                        { SCE_C_STRING, /*"字串",*/ STRINGSTYLE },
                        { SCE_C_STRINGEOL, /*"未关闭的字串",*/ NCLOSESTRING },
                        { SCE_C_CHARACTER, /*"字符",*/ STRINGSTYLE },
                        //{ SCE_C_UUID, "UUID", NULLSTYLE },
                        { SCE_C_OPERATOR, /*"运算符",*/ SETFG(MIDNIGHTBLUE) },
                        { SCE_C_PREPROCESSOR, /*"预处理器",*/ SETFG(DARKORANGE) },
                        //{ SCE_C_VERBATIM, "Verbatim", NULLSTYLE },
                        //{ SCE_C_REGEX, "Regex", NULLSTYLE },
                        //{ SCE_C_WORD2, "Word 2", NULLSTYLE },
                        //{ SCE_C_COMMENTDOCKEYWORD, "注释文档关键字", NULLSTYLE },
                        //{ SCE_C_COMMENTDOCKEYWORDERROR, "注释文档关键字错误", NULLSTYLE },
                        //{ SCE_C_GLOBALCLASS, "Global Class", NULLSTYLE },
                        EOSTYLE } };


KEYWORDLIST KeyWords_JAVA = {
            _T("abstract assert boolean break byte case catch char class const ")
            _T("continue default do double else extends final finally float for future ")
            _T("generic goto if implements import inner instanceof int interface long ")
            _T("native new null outer package private protected public rest return ")
            _T("short static super switch synchronized this throw throws transient try ")
            _T("var void volatile while"),
            _T(""), _T(""), _T(""), _T(""), _T(""), _T(""), _T(""), _T("") };


EDITLEXER lexJAVA = { SCLEX_CPP, IDS_LEXJAVA, /*"Java 源代码",*/ _T("java"), _T(""), II_JS, &KeyWords_JAVA, {
                          { STYLE_DEFAULT, /*"默认",*/ DEFAULTSTYLE },
                          //{ SCE_C_DEFAULT, "默认", NULLSTYLE },
                          { SCE_C_COMMENT, /*"注释",*/ COMMENTSTYLE },
                          { SCE_C_COMMENTLINE, /*"注释行",*/ COMMENTSTYLE },
                          { SCE_C_COMMENTDOC, /*"注释文档",*/ COMMENTSTYLE },
                          { SCE_C_COMMENTLINEDOC, /*"注释行文档",*/ COMMENTSTYLE },
                          { SCE_C_WORD, /*"关键字",*/ CPPKEYWORD },
                          { SCE_C_IDENTIFIER, /*"标识符",*/ NULLSTYLE },
                          { SCE_C_NUMBER, /*"数字",*/ SETFG(PURPLE) },
                          { SCE_C_STRING, /*"字串",*/ STRINGSTYLE },
                          { SCE_C_STRINGEOL, /*"未关闭的字串",*/ NCLOSESTRING },
                          { SCE_C_CHARACTER, /*"字符",*/ STRINGSTYLE },
                          //{ SCE_C_UUID, "UUID", NULLSTYLE },
                          { SCE_C_OPERATOR, /*"运算符",*/ SETFG(MIDNIGHTBLUE) },
                          { SCE_C_PREPROCESSOR, /*"预处理器",*/ SETFG(DARKORANGE) },
                          //{ SCE_C_VERBATIM, "Verbatim", NULLSTYLE },
                          //{ SCE_C_REGEX, "Regex", NULLSTYLE },
                          //{ SCE_C_WORD2, "Word 2", NULLSTYLE },
                          //{ SCE_C_COMMENTDOCKEYWORD, "注释文档关键字", NULLSTYLE },
                          //{ SCE_C_COMMENTDOCKEYWORDERROR, "注释文档关键字错误", NULLSTYLE },
                          //{ SCE_C_GLOBALCLASS, "Global Class", NULLSTYLE },
                          EOSTYLE } };


KEYWORDLIST KeyWords_PAS = {
           _T("and array asm begin case cdecl class const constructor default destructor ")
           _T("div do downto else end end. except exit exports external far file finalization ")
           _T("finally for function goto if implementation in index inherited initialization inline ")
           _T("interface label library message mod near nil not object of on or out overload ")
           _T("override packed pascal private procedure program property protected public ")
           _T("published raise read record register repeat resourcestring safecall set shl shr ")
           _T("stdcall stored string then threadvar to try type unit until uses var virtual ")
           _T("while with write xor"),
           _T("write read default public protected private property published stored"),
           _T(""), _T(""), _T(""), _T(""), _T(""), _T(""), _T("") };


EDITLEXER lexPAS = { SCLEX_PASCAL, IDS_LEXPAS, /*"Pascal 源代码",*/ _T("pas;dpr;dpk;dfm;inc;pp"), _T(""), II_MAIN, &KeyWords_PAS, {
                         { STYLE_DEFAULT, /*"默认",*/ DEFAULTSTYLE },
                         //{ 0, "默认", NULLSTYLE },
                         { SCE_C_COMMENT, /*"注释",*/ COMMENTSTYLE },
                         { SCE_C_COMMENTLINE, /*"注释行",*/ COMMENTSTYLE },
                         { SCE_C_COMMENTDOC, /*"注释文档",*/ COMMENTSTYLE },
                         { SCE_C_WORD, /*"关键字",*/ SETFG(PURPLE) },
                         { SCE_C_IDENTIFIER, /*"标识符",*/ NULLSTYLE },
                         //{ SCE_C_STRING, "字串", NULLSTYLE },
                         //{ SCE_C_STRINGEOL, "String eol", NULLSTYLE },
                         { SCE_C_NUMBER, /*"数字",*/ SETFG(PURPLE) },
                         { SCE_C_CHARACTER, /*"字串",*/ STRINGSTYLE },
                         { SCE_C_OPERATOR, /*"运算符", */SETFG(MIDNIGHTBLUE) },
                         { SCE_C_PREPROCESSOR, /*"预处理器",*/ STRINGSTYLE },
                         { SCE_C_REGEX, /*"Inline Asm",*/ SETFG(DARKORANGE) },
                         //{ SCE_C_COMMENTDOCKEYWORD, "注释文档关键字", COMMENTSTYLE },
                         //{ SCE_C_COMMENTDOCKEYWORDERROR, "注释文档关键字错误", COMMENTSTYLE },
                         EOSTYLE } };


KEYWORDLIST KeyWords_ASM = {
           _T("aaa aad aam aas adc add and call cbw clc cld cli cmc cmp cmps cmpsb ")
           _T("cmpsw cwd daa das dec div esc hlt idiv imul in inc int into iret ja jae jb jbe ")
           _T("jc jcxz je jg jge jl jle jmp jna jnae jnb jnbe jnc jne jng jnge jnl jnle jno jnp ")
           _T("jns jnz jo jp jpe jpo js jz lahf lds lea les lods lodsb lodsw loop loope loopew ")
           _T("loopne loopnew loopnz loopnzw loopw loopz loopzw mov movs movsb ")
           _T("movsw mul neg nop not or out pop popf push pushf rcl rcr ret retf retn rol ")
           _T("ror sahf sal sar sbb scas scasb scasw shl shr stc std sti stos stosb stosw ")
           _T("sub test wait xchg xlat xlatb xor bound enter ins insb insw leave outs ")
           _T("outsb outsw popa pusha pushw arpl lar lsl sgdt sidt sldt smsw str verr ")
           _T("verw clts lgdt lidt lldt lmsw ltr bsf bsr bt btc btr bts cdq cmpsd cwde insd ")
           _T("iretd iretdf iretf jecxz lfs lgs lodsd loopd looped loopned loopnzd loopzd ")
           _T("lss movsd movsx movzx outsd popad popfd pushad pushd pushfd scasd ")
           _T("seta setae setb setbe setc sete setg setge setl setle setna setnae ")
           _T("setnb setnbe setnc setne setng setnge setnl setnle setno setnp setns ")
           _T("setnz seto setp setpe setpo sets setz shld shrd stosd bswap cmpxchg ")
           _T("invd invlpg wbinvd xadd lock rep repe repne repnz repz cflush cpuid emms ")
           _T("femms cmovo cmovno cmovb cmovc cmovnae cmovae cmovnb cmovnc ")
           _T("cmove cmovz cmovne cmovnz cmovbe cmovna cmova cmovnbe cmovs ")
           _T("cmovns cmovp cmovpe cmovnp cmovpo cmovl cmovnge cmovge cmovnl ")
           _T("cmovle cmovng cmovg cmovnle cmpxchg486 cmpxchg8b loadall loadall286 ")
           _T("ibts icebp int1 int3 int01 int03 iretw popaw popfw pushaw pushfw rdmsr ")
           _T("rdpmc rdshr rdtsc rsdc rsldt rsm rsts salc smi smint smintold svdc svldt ")
           _T("svts syscall sysenter sysexit sysret ud0 ud1 ud2 umov xbts wrmsr wrshr"),

           _T("f2xm1 fabs fadd faddp fbld fbstp fchs fclex fcom fcomp fcompp fdecstp ")
           _T("fdisi fdiv fdivp fdivr fdivrp feni ffree fiadd ficom ficomp fidiv fidivr fild fimul ")
           _T("fincstp finit fist fistp fisub fisubr fld fld1 fldcw fldenv fldenvw fldl2e fldl2t ")
           _T("fldlg2 fldln2 fldpi fldz fmul fmulp fnclex fndisi fneni fninit fnop fnsave ")
           _T("fnsavew fnstcw fnstenv fnstenvw fnstsw fpatan fprem fptan frndint frstor ")
           _T("frstorw fsave fsavew fscale fsqrt fst fstcw fstenv fstenvw fstp fstsw fsub ")
           _T("fsubp fsubr fsubrp ftst fwait fxam fxch fxtract fyl2x fyl2xp1 fsetpm fcos ")
           _T("fldenvd fnsaved fnstenvd fprem1 frstord fsaved fsin fsincos fstenvd fucom ")
           _T("fucomp fucompp fcomi fcomip ffreep fcmovb fcmove fcmovbe fcmovu ")
           _T("fcmovnb fcmovne fcmovnbe fcmovnu"),

           _T("ah al ax bh bl bp bx ch cl cr0 cr2 cr3 cr4 cs cx dh di dl dr0 dr1 dr2 dr3 dr6 ")
           _T("dr7 ds dx eax ebp ebx ecx edi edx es esi esp fs gs si sp ss st tr3 tr4 tr5 ")
           _T("tr6 tr7 st0 st1 st2 st3 st4 st5 st6 st7 mm0 mm1 mm2 mm3 mm4 mm5 ")
           _T("mm6 mm7 xmm0 xmm1 xmm2 xmm3 xmm4 xmm5 xmm6 xmm7"),

           _T(".186 .286 .286c .286p .287 .386 .386c .386p .387 .486 .486p .8086 .8087 ")
           _T(".alpha .break .code .const .continue .cref .data .data? .dosseg .else ")
           _T(".elseif .endif .endw .err .err1 .err2 .errb .errdef .errdif .errdifi .erre .erridn ")
           _T(".erridni .errnb .errndef .errnz .exit .fardata .fardata? .if .lall .lfcond .list ")
           _T(".listall .listif .listmacro .listmacroall .model .no87 .nocref .nolist .nolistif ")
           _T(".nolistmacro .radix .repeat .sall .seq .sfcond .stack .startup .tfcond .type ")
           _T(".until .untilcxz .while .xall .xcref .xlist alias align assume catstr comm ")
           _T("comment db dd df dosseg dq dt dup dw echo else elseif elseif1 elseif2 ")
           _T("elseifb elseifdef elseifdif elseifdifi elseife elseifidn elseifidni elseifnb ")
           _T("elseifndef end endif endm endp ends eq equ even exitm extern externdef ")
           _T("extrn for forc ge goto group gt high highword if if1 if2 ifb ifdef ifdif ifdifi ife ")
           _T("ifidn ifidni ifnb ifndef include includelib instr invoke irp irpc label le length ")
           _T("lengthof local low lowword lroffset lt macro mask mod .msfloat name ne ")
           _T("offset opattr option org %out page popcontext proc proto ptr public ")
           _T("purge pushcontext record repeat rept seg segment short size sizeof ")
           _T("sizestr struc struct substr subtitle subttl textequ this title type typedef ")
           _T("union while width db dw dd dq dt resb resw resd resq rest incbin equ ")
           _T("times %define %idefine %xdefine %xidefine %undef %assign %iassign ")
           _T("%strlen %substr %macro %imacro %endmacro %rotate .nolist %if %elif ")
           _T("%else %endif %ifdef %ifndef %elifdef %elifndef %ifmacro %ifnmacro ")
           _T("%elifmacro %elifnmacro %ifctk %ifnctk %elifctk %elifnctk %ifidn %ifnidn ")
           _T("%elifidn %elifnidn %ifidni %ifnidni %elifidni %elifnidni %ifid %ifnid %elifid ")
           _T("%elifnid %ifstr %ifnstr %elifstr %elifnstr %ifnum %ifnnum %elifnum ")
           _T("%elifnnum %error %rep %endrep %exitrep %include %push %pop %repl ")
           _T("struct endstruc istruc at iend align alignb %arg %stacksize %local %line ")
           _T("bits use16 use32 section absolute extern global common cpu org section ")
           _T("group import export"),

           _T("$ ? @b @f addr basic byte c carry? dword far far16 fortran fword near ")
           _T("near16 overflow? parity? pascal qword real4 real8 real10 sbyte sdword ")
           _T("sign? stdcall sword syscall tbyte vararg word zero? flat near32 far32 abs ")
           _T("all assumes at casemap common compact cpu dotname emulator epilogue ")
           _T("error export expr16 expr32 farstack flat forceframe huge language large ")
           _T("listing ljmp loadds m510 medium memory nearstack nodotname noemulator ")
           _T("nokeyword noljmp nom510 none nonunique nooldmacros nooldstructs ")
           _T("noreadonly noscoped nosignextend nothing notpublic oldmacros oldstructs ")
           _T("os_dos para private prologue radix readonly req scoped setif2 smallstack ")
           _T("tiny use16 use32 uses # nasm directives, mostly complete, does not parse ")
           _T("properly a16 a32 o16 o32 byte word dword nosplit $ $$ seq wrt flat large ")
           _T("small .text .data .bss near far %0 %1 %2 %3 %4 %5 %6 %7 %8 %9"),

           _T("addpd addps addsd addss andpd andps andnpd andnps cmpeqpd cmpltpd ")
           _T("cmplepd cmpunordpd cmpnepd cmpnltpd cmpnlepd cmpordpd cmpeqps ")
           _T("cmpltps cmpleps cmpunordps cmpneps cmpnltps cmpnleps cmpordps ")
           _T("cmpeqsd cmpltsd cmplesd cmpunordsd cmpnesd cmpnltsd cmpnlesd ")
           _T("cmpordsd cmpeqss cmpltss cmpless cmpunordss cmpness cmpnltss ")
           _T("cmpnless cmpordss comisd comiss cvtdq2pd cvtdq2ps cvtpd2dq cvtpd2pi ")
           _T("cvtpd2ps cvtpi2pd cvtpi2ps cvtps2dq cvtps2pd cvtps2pi cvtss2sd ")
           _T("cvtss2si cvtsd2si cvtsd2ss cvtsi2sd cvtsi2ss cvttpd2dq cvttpd2pi ")
           _T("cvttps2dq cvttps2pi cvttsd2si cvttss2si divpd divps divsd divss fxrstor ")
           _T("fxsave ldmxscr lfence mfence maskmovdqu maskmovdq maxpd maxps ")
           _T("paxsd maxss minpd minps minsd minss movapd movaps movdq2q movdqa ")
           _T("movdqu movhlps movhpd movhps movd movq movlhps movlpd movlps ")
           _T("movmskpd movmskps movntdq movnti movntpd movntps movntq ")
           _T("movq2dq movsd movss movupd movups mulpd mulps mulsd mulss orpd ")
           _T("orps packssdw packsswb packuswb paddb paddsb paddw paddsw paddd ")
           _T("paddsiw paddq paddusb paddusw pand pandn pause paveb pavgb pavgw ")
           _T("pavgusb pdistib pextrw pcmpeqb pcmpeqw pcmpeqd pcmpgtb pcmpgtw ")
           _T("pcmpgtd pf2id pf2iw pfacc pfadd pfcmpeq pfcmpge pfcmpgt pfmax pfmin ")
           _T("pfmul pmachriw pmaddwd pmagw pmaxsw pmaxub pminsw pminub ")
           _T("pmovmskb pmulhrwc pmulhriw pmulhrwa pmulhuw pmulhw pmullw pmuludq ")
           _T("pmvzb pmvnzb pmvlzb pmvgezb pfnacc pfpnacc por prefetch prefetchw ")
           _T("prefetchnta prefetcht0 prefetcht1 prefetcht2 pfrcp pfrcpit1 pfrcpit2 ")
           _T("pfrsqit1 pfrsqrt pfsub pfsubr pi2fd pf2iw pinsrw psadbw pshufd pshufhw ")
           _T("pshuflw pshufw psllw pslld psllq pslldq psraw psrad psrlw psrld psrlq psrldq ")
           _T("psubb psubw psubd psubq psubsb psubsw psubusb psubusw psubsiw ")
           _T("pswapd punpckhbw punpckhwd punpckhdq punpckhqdq punpcklbw ")
           _T("punpcklwd punpckldq punpcklqdq pxor rcpps rcpss rsqrtps rsqrtss sfence ")
           _T("shufpd shufps sqrtpd sqrtps sqrtsd sqrtss stmxcsr subpd subps subsd ")
           _T("subss ucomisd ucomiss unpckhpd unpckhps unpcklpd unpcklps xorpd xorps"),
           _T(""), _T(""), _T("") };


EDITLEXER lexASM = { SCLEX_ASM, IDS_LEXASM, /*"汇编程序",*/ _T("asm"), _T(""), II_MAIN, &KeyWords_ASM, {
                         { STYLE_DEFAULT, /*"默认",*/ DEFAULTSTYLE },
                         //{ SCE_ASM_DEFAULT, "默认", NULLSTYLE },
                         { SCE_ASM_COMMENT, /*"注释",*/ COMMENTSTYLE },
                         //{ SCE_ASM_COMMENTBLOCK, "Comment Block (not used)", NULLSTYLE },
                         { SCE_ASM_NUMBER, /*"数字",*/ SETFG(PURPLE) },
                         { SCE_ASM_STRING, /*"双引号引用的字串",*/ COMMENTSTYLE },
                         { SCE_ASM_CHARACTER, /*"单引号引用的字串",*/ COMMENTSTYLE },
                         { SCE_ASM_STRINGEOL, /*"未关闭的字串",*/ NCLOSESTRING },
                         { SCE_ASM_OPERATOR, /*"运算符",*/ SETFG(MIDNIGHTBLUE) },
                         { SCE_ASM_IDENTIFIER, /*"标识符",*/ NULLSTYLE },
                         { SCE_ASM_CPUINSTRUCTION, /*"CPU 指令",*/ SETFG(MIDNIGHTBLUE) },
                         { SCE_ASM_MATHINSTRUCTION, /*"FPU 指令",*/ SETFG(MIDNIGHTBLUE) },
                         { SCE_ASM_REGISTER, /*"寄存器",*/ SETFG(DARKORANGE) },
                         { SCE_ASM_DIRECTIVE, /*"指令",*/ SETFG(MIDNIGHTBLUE) },
                         { SCE_ASM_DIRECTIVEOPERAND, /*"指令操作数",*/ SETFG(MIDNIGHTBLUE) },
                         { SCE_ASM_EXTINSTRUCTION, /*"扩展指令",*/ SETFG(MIDNIGHTBLUE) },
                         EOSTYLE } };



KEYWORDLIST KeyWords_PL = {
          _T("NULL __FILE__ __LINE__ __PACKAGE__ __DATA__ __END__ AUTOLOAD ")
          _T("BEGIN CORE DESTROY END EQ GE GT INIT LE LT NE CHECK abs accept alarm ")
          _T("and atan2 bind binmode bless caller chdir chmod chomp chop chown chr ")
          _T("chroot close closedir cmp connect continue cos crypt dbmclose dbmopen ")
          _T("defined delete die do dump each else elsif endgrent endhostent endnetent ")
          _T("endprotoent endpwent endservent eof eq eval exec exists exit exp fcntl ")
          _T("fileno flock for foreach fork format formline ge getc getgrent getgrgid ")
          _T("getgrnam gethostbyaddr gethostbyname gethostent getlogin getnetbyaddr ")
          _T("getnetbyname getnetent getpeername getpgrp getppid getpriority ")
          _T("getprotobyname getprotobynumber getprotoent getpwent getpwnam ")
          _T("getpwuid getservbyname getservbyport getservent getsockname ")
          _T("getsockopt glob gmtime goto grep gt hex if index int ioctl join keys kill last lc ")
          _T("lcfirst le length link listen local localtime lock log lstat lt m map mkdir msgctl ")
          _T("msgget msgrcv msgsnd my ne next no not oct open opendir or ord our pack ")
          _T("package pipe pop pos print printf prototype push q qq qr quotemeta qu qw ")
          _T("qx rand read readdir readline readlink readpipe recv redo ref rename require ")
          _T("reset return reverse rewinddir rindex rmdir s scalar seek seekdir select ")
          _T("semctl semget semop send setgrent sethostent setnetent setpgrp ")
          _T("setpriority setprotoent setpwent setservent setsockopt shift shmctl shmget ")
          _T("shmread shmwrite shutdown sin sleep socket socketpair sort splice split ")
          _T("sprintf sqrt srand stat study sub substr symlink syscall sysopen sysread ")
          _T("sysseek system syswrite tell telldir tie tied time times tr truncate uc ucfirst ")
          _T("umask undef unless unlink unpack unshift untie until use utime values vec ")
          _T("wait waitpid wantarray warn while write x xor y"),
          _T(""), _T(""), _T(""), _T(""), _T(""), _T(""), _T(""), _T("") };


EDITLEXER lexPL = { SCLEX_PERL, IDS_LEXPL, /*"Perl/CGI 脚本",*/ _T("pl;pm;cgi;pod"), _T(""), II_MAIN, &KeyWords_PL, {
                        { STYLE_DEFAULT, /*"默认",*/ DEFAULTSTYLE },
                        //{ SCE_PL_DEFAULT, "默认", NULLSTYLE },
                        { SCE_PL_ERROR, /*"错误",*/ NULLSTYLE },
                        { SCE_PL_COMMENTLINE, /*"注释",*/ COMMENTSTYLE },
                        { SCE_PL_NUMBER, /*"数字",*/ SETFG(PURPLE) },
                        { SCE_PL_WORD, /*"关键字",*/ SETFG(DARKORANGE) },
                        { SCE_PL_STRING, /*"双引号引用的字串",*/ COMMENTSTYLE },
                        { SCE_PL_CHARACTER, /*"单引号引用的字串",*/ COMMENTSTYLE },
                        //{ SCE_PL_PUNCTUATION, "Symbols / Punctuation (not used)", NULLSTYLE },
                        //{ SCE_PL_PREPROCESSOR, "Preprocessor (not used)", NULLSTYLE },
                        { SCE_PL_OPERATOR, /*"运算符",*/ NULLSTYLE },
                        { SCE_PL_IDENTIFIER, /*"标识符",*/ NULLSTYLE },
                        { SCE_PL_POD, /*"行开始的 POD",*/ NULLSTYLE },
                        { SCE_PL_SCALAR, /*"标量 $",*/ NULLSTYLE },
                        { SCE_PL_ARRAY, /*"数组 @",*/ NULLSTYLE },
                        { SCE_PL_HASH, /*"Hash %",*/ NULLSTYLE },
                        { SCE_PL_SYMBOLTABLE, /*"符号表 *",*/ NULLSTYLE },
                        { SCE_PL_REGEX, /*"Regex /re/ or m{re}",*/ NULLSTYLE },
                        { SCE_PL_REGSUBST, /*"Substitution s/re/or e/",*/ NULLSTYLE },
                        //{ SCE_PL_LONGQUOTE, "Long Quote (qq, qr, qw, qx) (not used)", NULLSTYLE },
                        { SCE_PL_BACKTICKS, /*"Back Ticks",*/ NULLSTYLE },
                        { SCE_PL_DATASECTION, /*"数据区",*/ NULLSTYLE },
                        { SCE_PL_HERE_DELIM, /*"Here-doc (Delimiter)",*/ NULLSTYLE },
                        { SCE_PL_HERE_Q, /*"Here-doc (Single quoted, q)",*/ NULLSTYLE },
                        { SCE_PL_HERE_QQ, /*"Here-doc (Double quoted, qq)",*/ NULLSTYLE },
                        { SCE_PL_HERE_QX, /*"Here-doc (Back ticks, qx)",*/ NULLSTYLE },
                        { SCE_PL_STRING_Q, /*"Single quoted string, generic (q)",*/ NULLSTYLE },
                        { SCE_PL_STRING_QQ, /*"双引字串 (qq)",*/ NULLSTYLE },
                        { SCE_PL_STRING_QX, /*"Back ticks (qx)",*/ NULLSTYLE },
                        { SCE_PL_STRING_QR, /*"Regex (qr)",*/ NULLSTYLE },
                        { SCE_PL_STRING_QW, /*"数组 (qw)",*/ NULLSTYLE },
                        EOSTYLE } };


KEYWORDLIST KeyWords_INI = {
                               _T(""), _T(""), _T(""), _T(""), _T(""), _T(""), _T(""), _T(""), _T("") };


EDITLEXER lexINI = { SCLEX_PROPERTIES, IDS_LEXINI, /*"配置文件",*/ _T("cfg;ini;inf;properties"), _T(""), II_INI, &KeyWords_INI, {
                         { STYLE_DEFAULT, /*"默认",*/ DEFAULTSTYLE },
                         //{ SCE_PROPS_DEFAULT, "默认", NULLSTYLE },
                         { SCE_PROPS_COMMENT, /*"注释",*/ COMMENTSTYLE },
                         { SCE_PROPS_SECTION, /*"区段",*/ SETEOL(0, 0xECF5FF) },
                         { SCE_PROPS_ASSIGNMENT, /*"任务",*/ SETFG(RED) },
                         { SCE_PROPS_DEFVAL, /*"默认值",*/ SETFG(RED) },
                         EOSTYLE } };


EDITLEXER lexREG = { SCLEX_PROPERTIES, IDS_LEXREG, /*"注册表文件",*/ _T("reg;url"), _T(""), II_NONE, &KeyWords_INI, {
                         { STYLE_DEFAULT, /*"默认",*/ DEFAULTSTYLE },
                         //{ SCE_PROPS_DEFAULT, "默认", NULLSTYLE },
                         { SCE_PROPS_COMMENT, /*"注释",*/ COMMENTSTYLE },
                         { SCE_PROPS_SECTION, /*"区段",*/ SETEOL(0, 0xECF5FF) },
                         { SCE_PROPS_ASSIGNMENT, /*"任务",*/ SETFG(RED) },
                         { SCE_PROPS_DEFVAL, /*"默认值",*/ SETFG(RED) },
                         EOSTYLE } };


KEYWORDLIST KeyWords_BAT = {
           _T("rem set if exist errorlevel for in do break call chcp cd chdir choice cls ")
           _T("country ctty date del erase dir echo exit goto loadfix loadhigh mkdir md ")
           _T("move path pause prompt rename ren rmdir rd shift time type ver verify vol ")
           _T("com con lpt nul echo."),
           _T(""), _T(""), _T(""), _T(""), _T(""), _T(""), _T(""), _T("") };


EDITLEXER lexBAT = { SCLEX_BATCH, IDS_LEXBAT, /*"批处理文件",*/ _T("bat;cmd"), _T(""), II_NONE, &KeyWords_BAT, {
                         { STYLE_DEFAULT, /*"默认",*/ DEFAULTSTYLE },
                         //{ SCE_BAT_DEFAULT, "默认", NULLSTYLE },
                         { SCE_BAT_COMMENT, /*"注释",*/ COMMENTSTYLE },
                         { SCE_BAT_WORD, /*"关键字",*/ CPPKEYWORD },
                         { SCE_BAT_LABEL, /*"标记",*/ SETEOL(DARKBLUE, 0xEEFFFF) },
                         { SCE_BAT_HIDE, /*"隐藏",*/ NULLSTYLE },
                         { SCE_BAT_COMMAND, /*"命令",*/ NULLSTYLE },
                         { SCE_BAT_IDENTIFIER, /*"标识符",*/ NULLSTYLE },
                         { SCE_BAT_OPERATOR, /*"运算符",*/ NULLSTYLE },
                         EOSTYLE } };


KEYWORDLIST KeyWords_DIFF = {
                                _T(""), _T(""), _T(""), _T(""), _T(""), _T(""), _T(""), _T(""), _T("") };


EDITLEXER lexDIFF = { SCLEX_DIFF, IDS_LEXDIFF, /*"Diff 文件",*/ _T("diff;patch"), _T(""), II_MAIN, &KeyWords_DIFF, {
                          { STYLE_DEFAULT, /*"默认",*/ DEFAULTSTYLE },
                          //{ SCE_DIFF_DEFAULT, "默认", NULLSTYLE },
                          { SCE_DIFF_COMMENT, /*"注释",*/ COMMENTSTYLE },
                          { SCE_DIFF_COMMAND, /*"命令",*/ SETFG(MIDNIGHTBLUE) },
                          { SCE_DIFF_HEADER, /*"源和目标文件",*/ SETEOL(MAROON, AQUA) },
                          { SCE_DIFF_POSITION, /*"位置设置",*/ SETFG(RED) },
                          { SCE_DIFF_ADDED, /*"添加的行",*/ SETEOL(0, 0x80FF80) },
                          { SCE_DIFF_DELETED, /*"移除的行",*/ SETEOL(0, 0x8080FF) },
                          EOSTYLE } };


KEYWORDLIST KeyWords_SQL = {
           _T("absolute action add admin after aggregate alias all allocate alter and any ")
           _T("are array as asc assertion at authorization before begin binary bit blob ")
           _T("boolean both breadth by call cascade cascaded case cast catalog char ")
           _T("character check class clob close collate collation column commit completion ")
           _T("connect connection constraint constraints constructor continue ")
           _T("corresponding create cross cube current current_date current_path ")
           _T("current_role current_time current_timestamp current_user cursor cycle data ")
           _T("date day deallocate dec decimal declare default deferrable deferred delete ")
           _T("depth deref desc describe descriptor destroy destructor deterministic ")
           _T("dictionary diagnostics disconnect distinct domain double drop dynamic each ")
           _T("else end end-exec equals escape every except exception exec execute ")
           _T("external false fetch first float for foreign found from free full function general ")
           _T("get global go goto grant group grouping having host hour identity if ignore ")
           _T("immediate in indicator initialize initially inner inout input insert int integer ")
           _T("intersect interval into is isolation iterate join key language large last lateral ")
           _T("leading left less level like limit local localtime localtimestamp locator map ")
           _T("match minute modifies modify module month names national natural nchar ")
           _T("nclob new next no none not null numeric object of off old on only open ")
           _T("operation option or order ordinality out outer output pad parameter ")
           _T("parameters partial path postfix precision prefix preorder prepare preserve ")
           _T("primary prior privileges procedure public read reads real recursive ref ")
           _T("references referencing relative restrict result return returns revoke right role ")
           _T("rollback rollup routine row rows savepoint schema scroll scope search ")
           _T("second section select sequence session session_user set sets size smallint ")
           _T("some space specific specifictype sql sqlexception sqlstate sqlwarning start ")
           _T("state statement static structure system_user table temporary terminate ")
           _T("than then time timestamp timezone_hour timezone_minute to trailing ")
           _T("transaction translation treat trigger true under union unique unknown unnest ")
           _T("update usage user using value values varchar variable varying view when ")
           _T("whenever where with without work write year zone"),
           _T(""), _T(""), _T(""), _T(""), _T(""), _T(""), _T(""), _T("") };


EDITLEXER lexSQL = { SCLEX_SQL, IDS_LEXSQL, /*"SQL 查询",*/ _T("sql"), _T(""), II_SQL, &KeyWords_SQL, {
                         //{ 0, "默认", NULLSTYLE },
                         { STYLE_DEFAULT, /*"默认",*/ DEFAULTSTYLE },
                         { 1, /*"注释",*/ COMMENTSTYLE },
                         { 2, /*"行注释",*/ COMMENTSTYLE },
                         { 3, /*"文档注释",*/ COMMENTSTYLE },
                         { 4, /*"数字",*/ SETFG(RED) },
                         { 5, /*"关键字",*/ SETFG(PURPLE) },
                         { 6, /*"双引号引用的字串",*/ STRINGSTYLE },
                         { 7, /*"单引号引用的字串",*/ STRINGSTYLE },
                         //{ 12, "未关闭的字串", NCLOSESTRING },
                         { 10, /*"运算符",*/ NULLSTYLE },
                         { 11, /*"标识符",*/ NULLSTYLE },
                         //{ 8, "Symbols", NULLSTYLE },
                         //{ 9, "预处理器", SETFG(DARKORANGE) },
                         EOSTYLE } };


KEYWORDLIST KeyWords_AS = {
          _T("arguments break constructor case class continue default delete dynamic false else extends for ")
          _T("function if implements import in instanceof interface intrinsic new newline null private ")
          _T("public return super static switch this true typeof undefined var void while with ")
          _T("Accessibility Arguments Array Boolean Button Camera ContextMenu ContextMenuItem CustomActions ")
          _T("Color Date Error Function Key LoadVars LocalConnection Math Microphone Mouse MovieClip ")
          _T("MovieClipLoader NetConnection NetStream Number PrintJob Object TextField StyleSheet ")
          _T("TextFormat TextSnapshot SharedObject Selection Sound Stage String System XML XMLNode ")
          _T("XMLSocket Void abs acos asin atan atan2 ceil cos exp floor log max min pow random round sin ")
          _T("sqrt tan on onActivity onChanged onClipEvent onClose onConnect onData onDragOut onDragOver ")
          _T("onEnterFrame onID3 onKeyDown onKeyUp onKillFocus onLoad onLoadComplete onLoadError onLoadInit ")
          _T("onLoadProgress onLoadStart onMouseDown onMouseMove onMouseUp onMouseWheel onPress onRelease ")
          _T("onReleaseOutside onResize onRollOut onRollOver onScroller onSelect onSetFocus onSoundComplete ")
          _T("onStatus onUnload onUpdate onXML add addListener addPage addProperty addRequestHeader ")
          _T("allowDomain allowInsecureDomain appendChild apply applyChanges asfunction attachAudio ")
          _T("attachMovie attachSound attachVideo beginFill beginGradientFill call ceil charAt charCodeAt ")
          _T("clear clearInterval cloneNode close concat connect copy cos createElement ")
          _T("createEmptyMovieClip createTextField createTextNode curveTo domain duplicateMovieClip endFill ")
          _T("escape eval evaluate exp findText floor fscommand flush fromCharCode get getAscii ")
          _T("getBeginIndex getBounds getBytesLoaded getBytesTotal getCaretIndex getCode getCount getDate ")
          _T("getDay getDepth getEndIndex getFocus getFontList getFullYear getHours getInstanceAtDepth ")
          _T("getLocal getMilliseconds getMinutes getMonth getNewTextFormat getNextHighestDepth getPan ")
          _T("getProggress getProperty getRGB getSeconds getSelected getSelectedText getSize getStyle ")
          _T("getStyleNames getSWFVersion getText getTextExtent getTextFormat getTextSnapshot getTime ")
          _T("getTimer getTimezoneOffset getTransform getURL getUTCDate getUTCDay getUTCFullYear ")
          _T("getUTCHours getUTCMilliseconds getUTCMinutes getUTCMonth getUTCSeconds getVersion getVolume ")
          _T("getYear globalToLocal gotoAndPlay gotoAndStop hasChildNodes hide hideBuiltInItems hitTest ")
          _T("hitTestTextNearPos indexOf insertBefore install isActive isDown isToggled join lastIndexOf ")
          _T("lineStyle lineTo list load loadClip loadMovie loadMovieNum loadSound loadVariables ")
          _T("loadVariablesNum localToGlobal log mbchr mblength mbord mbsubstring min MMExecute moveTo ")
          _T("nextFrame nextScene parseCSS parseFloat parseInt parseXML pause play pop pow prevScene print ")
          _T("printAsBitmap printAsBitmapNum printNum push random registerClass removeListener ")
          _T("removeMovieClip removeNode removeTextField replaceSel replaceText reverse round seek send ")
          _T("sendAndLoad setBufferTime set setDate setFocus setFullYear setGain setHours setInterval ")
          _T("setMask setMilliseconds setMinutes setMode setMonth setMotionLevel setNewTextFormat setPan ")
          _T("setProperty setQuality setRate setRGB setSeconds setSelectColor setSelected setSelection ")
          _T("setSilenceLevel setStyle setTextFormat setTime setTransform setUseEchoSuppression setUTCDate ")
          _T("setUTCFullYear setUTCHours setUTCMilliseconds setUTCMinutes setUTCMonth setUTCSeconds ")
          _T("setVolume setYear shift show showSettings silenceLevel silenceTimeout sin slice sort sortOn ")
          _T("splice split sqrt start startDrag stop stopAllSounds stopDrag substr substring swapDepths tan ")
          _T("toggleHighQuality toLowerCase toString toUpperCase trace unescape uninstall unLoadClip ")
          _T("unloadMovie unloadMovieNum unshift unwatch updateAfterEvent updateProperties ")
          _T("useEchoSuppression valueOf watch endinitclip include initclip __proto__ _accProps _alpha ")
          _T("_currentframe _droptarget _focusrect _framesloaded _global _height _highquality _level ")
          _T("_lockroot _name _parent _quality _root _rotation _soundbuftime _target _totalframes _url ")
          _T("_visible _width _x _xmouse _xscale _y _ymouse _yscale activityLevel align allowDomain ")
          _T("allowInsecureDomain attributes autoSize avHardwareDisable background backgroundColor ")
          _T("bandwidth blockIndent bold border borderColor bottomScroll bufferLenght bufferTime ")
          _T("builtInItems bullet bytesLoaded bytesTotal callee caller capabilities caption childNodes ")
          _T("color condenseWhite contentType currentFps customItems data deblocking docTypeDecl duration ")
          _T("embedFonts enabled exactSettings firstChild focusEnabled font fps gain globalStyleFormat ")
          _T("hasAccessibility hasAudio hasAudioEncoder hasEmbeddedVideo hasMP3 hasPrinting ")
          _T("hasScreenBroadcast hasScreenPlayback hasStreamingAudio hasStreamingVideo hasVideoEncoder ")
          _T("height hitArea hscroll html htmlText indent index italic instanceof int ignoreWhite ")
          _T("isDebugger isDown isFinite italic language lastChild leading leftMargin length loaded ")
          _T("localFileReadDisable manufacturer maxChars maxhscroll maxscroll menu message motionLevel ")
          _T("motionTimeout mouseWheelEnabled multiline muted name names NaN nextSibling nodeName nodeType ")
          _T("nodeValue os parentNode password pixelAspectRatio playerType previousSibling prototype ")
          _T("quality rate restrict resolutionX resolutionY rightMargin scaleMode screenColor screenDPI ")
          _T("screenResolutionX screenResolutionY scroll selectable separatorBefore showMenu size smoothing ")
          _T("status styleSheet tabChildren tabEnabled tabIndex tabStops target targetPath text textColor ")
          _T("textHeight textWidth time trackAsMenu type typeof underline url useCodepage ")
          _T("useEchoSuppression useHandCursor variable version visible width wordWrap xmlDecl"),
          _T(""), _T(""), _T(""), _T(""), _T(""), _T(""), _T(""), _T("") };


EDITLEXER lexAS = { SCLEX_CPP, IDS_LEXAS, /*"ActionScript 2.0",*/ _T("as;mx"), _T(""), II_NONE, &KeyWords_AS, {
                        { STYLE_DEFAULT, /*"默认",*/ DEFAULTSTYLE },
                        //{ SCE_C_DEFAULT, "默认", NULLSTYLE },
                        { SCE_C_COMMENT, /*"注释",*/ SETSIZEFG(0xFF000002, 0x8000FF) },
                        { SCE_C_COMMENTLINE, /*"注释行",*/ SETSIZEFG(0xFF000002, 0x8000FF) },
                        { SCE_C_COMMENTDOC, /*"注释文档",*/ SETSIZEFG(0xFF000002, 0x8000FF) },
                        { SCE_C_COMMENTLINEDOC, /*"注释行文档",*/ SETSIZEFG(0xFF000002, 0x8000FF) },
                        { SCE_C_WORD, /*"关键字",*/ CPPKEYWORD },
                        { SCE_C_IDENTIFIER, /*"标识符",*/ NULLSTYLE },
                        { SCE_C_NUMBER, /*"数字",*/ SETFG(PURPLE) },
                        { SCE_C_STRING, /*"字串",*/ STRINGSTYLE },
                        { SCE_C_STRINGEOL, /*"未关闭的字串",*/ NCLOSESTRING },
                        { SCE_C_CHARACTER, /*"字符",*/ STRINGSTYLE },
                        //{ SCE_C_UUID, "UUID", NULLSTYLE },
                        { SCE_C_OPERATOR, /*"运算符",*/ SETFG(RED) },
                        { SCE_C_PREPROCESSOR, /*"预处理器",*/ STRINGSTYLE },
                        //{ SCE_C_VERBATIM, "Verbatim", NULLSTYLE },
                        //{ SCE_C_REGEX, "Regex", NULLSTYLE },
                        //{ SCE_C_WORD2, "Word 2", NULLSTYLE },
                        //{ SCE_C_COMMENTDOCKEYWORD, "注释文档关键字", NULLSTYLE },
                        //{ SCE_C_COMMENTDOCKEYWORDERROR, "注释文档关键字错误", NULLSTYLE },
                        //{ SCE_C_GLOBALCLASS, "Global Class", NULLSTYLE },
                        EOSTYLE } };


KEYWORDLIST KeyWords_NSIS = {
            //指令
            //声明变量
            _T("Var ")
            //页面
            _T("Page UninstPage PageEx PageExEnd PageCallbacks ")
            //区段
            _T("AddSize Section SectionEnd SectionIn SubSection SubSectionEnd SectionGroup SectionGroup ")
            //函数
            _T("Function FunctionEnd ")
            //安装回调
            _T(".onGUIInit .onInit .onInstFailed .onInstSuccess .onGUIEnd .onMouseOverSection .onSelChange ")
            _T(".onUserAbort .onVerifyInstDir ")
            //卸载回调
            _T("un.onGUIInit un.onInit .onUninstFailed .onUninstSuccess un.onGUIEnd un.onUserAbort ")
            //常规属性
            _T("AddBrandingImage AllowRootDirInstall AutoCloseWindow BGGradient BrandingText Caption ")
            _T("ChangeUI CheckBitmap CompletedText ComponentText CRCCheck DetailsButtonText DirText ")
            _T("DirVar DirVerify FileErrorText Icon InstallButtonText InstallColors InstallDir ")
            _T("InstallDirRegKey InstProgressFlags InstType LicenseBkColor LicenseData LicenseForceSelection ")
            _T("LicenseText MiscButtonText Name OutFile SetFont ShowInstDetails ShowUninstDetails ")
            _T("SilentInstall SilentUnInstall SpaceTexts SubCaption UninstallButtonText UninstallCaption ")
            _T("UninstallIcon UninstallSubCaption UninstallText WindowIcon XPStyle ")
            //编译器位标计
            _T("AllowSkipFiles FileBufSize SetCompress SetCompressor SetCompressorDictSize ")
            _T("SetDatablockOptimize SetDateSave SetOverwrite SetPluginUnload ")
            //版本信息
            _T("VIAddVersionKey VIProductVersion ")
            //基本指令
            _T("Delete Exec ExecShell ExecWait File Rename ReserveFile RMDir SetOutPath ")
            //Registry, INI, File Instructions
            _T("DeleteINISec DeleteINIStr DeleteRegKey DeleteRegValue EnumRegKey EnumRegValue ")
            _T("ExpandEnvStrings FlushINI ReadEnvStr ReadINIStr ReadRegDWORD ReadRegStr WriteINIStr ")
            _T("WriteRegBin WriteRegDWORD WriteRegStr WriteRegExpandStr ")
            //General Purpose Instructions
            _T("CallInstDLL CopyFiles CreateDirectory CreateShortCut GetDLLVersion GetDLLVersionLocal ")
            _T("GetFileTime GetFileTimeLocal GetFullPathName GetTempFileName SearchPath SetFileAttributes ")
            _T("RegDLL UnRegDLL ")
            //Flow Control Instructions
            _T("Abort Call ClearErrors GetCurrentAddress GetFunctionAddress GetLabelAddress Goto IfAbort ")
            _T("IfErrors IfFileExists IfRebootFlag IfSilent IntCmp IntCmpU MessageBox Return Quit SetErrors StrCmp ")
            //File Instructions
            _T("FileClose FileOpen FileRead FileReadByte FileSeek FileWrite FileWriteByte FindClose FindFirst FindNext ")
            //Uninstaller Instructions
            _T("WriteUninstaller ")
            //Miscellaneous Instructions
            _T("GetErrorLevel GetInstDirError InitPluginsDir SetErrorLevel SetShellVarContext Sleep ")
            //String Manipulation Instructions
            _T("StrCpy StrLen ")
            //Stack Support
            _T("Exch Pop Push ")
            //Integer Support
            _T("IntFmt IntOp ")
            //Reboot Instructions
            _T("Reboot SetRebootFlag ")
            //Install Logging Instructions
            _T("LogSet LogText ")
            //Section Management
            _T("SectionSetFlags SectionGetFlags SectionSetText SectionGetText SectionSetInstTypes ")
            _T("SectionGetInstTypes SectionSetSize SectionGetSize SetCurInstType GetCurInstType ")
            _T("InstTypeSetText InstTypeGetText ")
            //User Interface Instructions
            _T("BringToFront CreateFont DetailPrint EnableWindow FindWindow GetDlgItem HideWindow ")
            _T("IsWindow SendMessage SetAutoClose SetBrandingImage SetDetailsView SetDetailsPrint ")
            _T("SetCtlColors SetSilent ShowWindow ")
            //Multiple Languages Instructions
            _T("LoadLanguageFile LangString LicenseLangString ")
            //Compiler Utility Commands
            _T("!include !addincludedir !addplugindir !cd !echo !error !execute !packhdr !system !warning !verbose ")
			_T("!appendfile !delfile !tempfile ")
            //Conditional Compilation
            _T("!define !undef !ifdef !ifndef !ifmacrodef !ifmacrondef !else !endif !insertmacro !macro ")
            _T("!macroend ")
            //Hiden Instructions
            _T("LockWindow "),
            //变量
            _T("$0 $1 $2 $3 $4 $5 $6 $7 $8 $9 $R $R0 $R1 $R2 $R3 $R4 $R5 $R6 $R7 $R8 $R9 ")
            _T("$INSTDIR $OUTDIR $CMDLINE $LANGUAGE $HWND ")
            _T("$PROGRAMFILES $COMMONFILES $DESKTOP $EXEDIR $WINDIR $SYSDIR $TEMP $STARTMENU $SMPROGRAMS ")
            _T("$SMSTARTUP $QUICKLAUNCH $DOCUMENTS $SENDTO $RECENT $FAVORITES $MUSIC $PICTURES $VIDEOS ")
            _T("$NETHOOD $FONTS $TEMPLATES $APPDATA $PRINTHOOD $INTERNET_CACHE $COOKIES $HISTORY $PROFILE ")
            _T("$ADMINTOOLS $RESOURCES $RESOURCES_LOCALIZED $CDBURN_AREA $HWNDPARENT ")
            _T("$PLUGINSDIR $MSG $WPARAM $LPARAM"),
            //标记
            _T("on off true false custom license components directory instfiles uninstConfirm un.custom ")
            _T("un.license un.components un.directory un.instfiles un.uninstConfirm Uninstall left right ")
            _T("top bottom notext LEFT RIGHT CENTER IDD_LICENSE IDD_DIR IDD_SELCOM IDD_INST DD_INSTFILES ")
            _T("IDD_UNINST  IDD_VERIFY force auto leave windows NOCUSTOM CUSTOMSTRING COMPONENTSONLYONCUSTOM ")
            _T("gray checkbox radiobuttons hide show nevershow normal silent silentlog zlib bzip2 lzma try ")
            _T("ifnewer ifdiff lastused manual alwaysoff LANG ProductName Comments CompanyName LegalCopyright ")
            _T("FileDescription FileVersion ProductVersion InternalName LegalTrademarks OriginalFilename ")
            _T("PrivateBuild SpecialBuild REBOOTOK nonfatal oname ifempty HKCR HKEY_CLASSES_ROOT HKLM ")
            _T("HKEY_LOCAL_MACHINE HKCU HKEY_CURRENT_USER HKU HKEY_USERS HKCC HKEY_CURRENT_CONFIG HKDD ")
            _T("HKEY_DYN_DATA HKPD HKEY_PERFORMANCE_DATA NOUNLOAD SILENT FILESONLY SW_SHOWNORMAL ")
            _T("SW_SHOWMAXIMIZED SW_SHOWMINIMIZED ALT CONTROL EXT SHIFT SHORT NORMAL FILE_ATTRIBUTE_NORMAL ")
            _T("ARCHIVE FILE_ATTRIBUTE_ARCHIVE HIDDEN FILE_ATTRIBUTE_HIDDEN OFFLINE FILE_ATTRIBUTE_OFFLINE ")
            _T("READONLY FILE_ATTRIBUTE_READONLY SYSTEM FILE_ATTRIBUTE_SYSTEM TEMPORARY ")
            _T("FILE_ATTRIBUTE_TEMPORARY MB_OK MB_OKCANCEL MB_ABORTRETRYIGNORE MB_RETRYCANCEL MB_YESNO ")
            _T("MB_YESNOCANCEL MB_ICONEXCLAMATION MB_ICONINFORMATION MB_ICONQUESTION MB_ICONSTOP MB_TOPMOST ")
            _T("MB_SETFOREGROUND MB_RIGHT MB_DEFBUTTON1 MB_DEFBUTTON2 MB_DEFBUTTON3 MB_DEFBUTTON4 IDABORT ")
            _T("IDCANCEL IDIGNORE IDNO IDOK IDRETRY IDYES SD SET CUR END current all ITALIC UNDERLINE STRIKE ")
            _T("TIMEOUT IMGID RESIZETOFIT none listonly textonly both lastused BRANDING transparent")
            ,
            _T(""), _T(""), _T(""), _T(""), _T(""), _T("") };


EDITLEXER lexNSIS = { SCLEX_NSIS, IDS_LEXNSIS, /*"NSIS 脚本",*/ _T("nsi;nsh"), _T(""), II_NSIS, &KeyWords_NSIS, {
              { STYLE_DEFAULT, /*"默认",*/ DEFAULTSTYLE },
              //{ SCE_NSIS_DEFAULT, "默认", NULLSTYLE },
              { SCE_NSIS_COMMENT, /*"单行注释",*/ COMMENTSTYLE },
              { SCE_NSIS_COMMENTBOX, /*"多行注释",*/ COMMENTSTYLE },
              { SCE_NSIS_STRINGDQ, /*"双引号引用的字串",*/ STRINGSTYLE },
              { SCE_NSIS_STRINGLQ, /*"左引号(`)引用的字串",*/ STRINGSTYLE },
              { SCE_NSIS_STRINGRQ, /*"右引号(')引用的字串",*/ STRINGSTYLE },
              { SCE_NSIS_FUNCTION, /*"指令",*/ CPPKEYWORD },
              { SCE_NSIS_VARIABLE, /*"变量",*/ SETFG(FIREBRICK) },
              { SCE_NSIS_STRINGVAR, /*"字串里的变量",*/ SETFG(FIREBRICK) },
              { SCE_NSIS_NUMBER, /*"数字",*/ SETFG(PURPLE) },
              { SCE_NSIS_LABEL, /*"标记",*/ DARKORANGE },
              { SCE_NSIS_USERDEFINED, /*"用户定义的",*/ SETFG(FIREBRICK) },
              { SCE_NSIS_SECTIONDEF, /*"区段",*/ CPPKEYWORD },
              { SCE_NSIS_FUNCTIONDEF, /*"函数",*/ CPPKEYWORD },
              { SCE_NSIS_SUBSECTIONDEF, /*"子区段",*/ SETFG(DARKBLUE) },
              { SCE_NSIS_SECTIONGROUP, /*"区段组",*/ SETFG(DARKBLUE) },
              { SCE_NSIS_PAGEEX, /*"页面扩展指令",*/ CPPKEYWORD },
              { SCE_NSIS_IFDEFINEDEF, /*"条件定义",*/ CPPKEYWORD },
              { SCE_NSIS_MACRODEF, /*"宏定义",*/ SETFG(DARKBLUE) },
              EOSTYLE } };


KEYWORDLIST KeyWords_PY = {
          _T("and assert break class continue def del elif else except ")
          _T("exec finally for from global if import in is lambda None ")
          _T("not or pass print raise return try while yield"),
          _T(""), _T(""), _T(""), _T(""), _T(""), _T(""), _T(""), _T("") };


EDITLEXER lexPY = { SCLEX_PYTHON, IDS_LEXPY, /*"Python 脚本",*/ _T("py;pyw"), _T(""), II_PY, &KeyWords_PY, {
                        { STYLE_DEFAULT, /*"默认",*/ DEFAULTSTYLE },
                        //{ SCE_P_DEFAULT, "默认", NULLSTYLE },
                        { SCE_P_COMMENTLINE, /*"注释",*/ SETFG(0x007F00) },
                        { SCE_P_COMMENTBLOCK, /*"注释区块",*/ SETFG(0x007F00) },
                        { SCE_P_WORD, /*"关键字",*/ SETFG(0x7F0000) },
                        { SCE_P_IDENTIFIER, /*"标识符",*/ NULLSTYLE },
                        { SCE_P_NUMBER, /*"数字",*/ SETFG(0x7F7F00) },
                        { SCE_P_OPERATOR, /*"运算符",*/ NULLSTYLE },
                        { SCE_P_STRING, /*"双引号引用的字串",*/ SETFG(DARKORANGE) },
                        { SCE_P_CHARACTER, /*"单引号引用的字串",*/ SETFG(DARKORANGE) },
                        { SCE_P_STRINGEOL, /*"未关闭的字串",*/ NCLOSESTRING },
                        { SCE_P_TRIPLEDOUBLE, /*"双引字串",*/ SETFG(DARKORANGE) },
                        { SCE_P_TRIPLE, /*"单引字串",*/ SETFG(DARKORANGE) },
                        { SCE_P_CLASSNAME, /*"类名",*/ SETFG(BLUE) },
                        { SCE_P_DEFNAME, /*"函数名",*/ SETFG(0x7F7F00) },
                        EOSTYLE } };


KEYWORDLIST KeyWords_RUBY = {
          _T("FILE__ and def end in or self unless __LINE__ begin ")
		  _T("defined ensure module redo super until BEGIN break do ")
		  _T("false next rescue then when END case else for nil retry ")
		  _T("true while alias class elsif if not return undef yield"),
          _T(""), _T(""), _T(""), _T(""), _T(""), _T(""), _T(""), _T("") };


EDITLEXER lexRUBY = { SCLEX_RUBY, IDS_LEXRUBY, /*"Ruby 脚本",*/ _T("rb;rbw"), _T(""), II_MAK, &KeyWords_RUBY, {
                        { STYLE_DEFAULT, /*"默认",*/ DEFAULTSTYLE },
                        { SCE_RB_COMMENTLINE, /*"注释行",*/ COMMENTSTYLE },
                        { SCE_RB_POD, /*"POD",*/ SETEOL(0, 0) },
                        { SCE_RB_NUMBER, /*"数字",*/ SETFG(PURPLE) },
                        { SCE_RB_STRING, /*"字串",*/ STRINGSTYLE },
                        { SCE_RB_OPERATOR, /*"运算符",*/ SETFG(DARKBLUE) },
                        EOSTYLE } };


KEYWORDLIST KeyWords_FORTRAN = {
          _T("access action advance allocatable allocate apostrophe assign assignment associate ")
		  _T("asynchronous backspace bind blank blockdata call case character class close common complex ")
		  _T("contains continue cycle data deallocate decimal delim default dimension direct do dowhile ")
		  _T("double doubleprecision else elseif elsewhere encoding end endassociate endblockdata ")
		  _T("enddo endfile endforall endfunction endif endinterface endmodule endprogram endselect ")
		  _T("endsubroutine endtype endwhere entry eor equivalence err errmsg exist exit external file ")
		  _T("flush fmt forall form format formatted function go goto id if implicit in include inout ")
		  _T("integer inquire intent interface intrinsic iomsg iolength iostat kind len logical module ")
		  _T("name named namelist nextrec nml none nullify number only open opened operator optional out ")
		  _T("pad parameter pass pause pending pointer pos position precision print private program ")
		  _T("protected public quote read readwrite real rec recl recursive result return rewind save select ")
		  _T("selectcase selecttype sequential sign size stat status stop stream subroutine target then to ")
		  _T("type unformatted unit use value volatile wait where while write"),

          _T("abs achar acos acosd adjustl adjustr aimag aimax0 aimin0 aint ajmax0 ajmin0 akmax0 akmin0 all ")
		  _T("allocated alog alog10 amax0 amax1 amin0 amin1 amod anint any asin asind associated atan atan2 atan2d ")
		  _T("atand bitest bitl bitlr bitrl bjtest bit_size bktest break btest cabs ccos cdabs cdcos cdexp cdlog ")
		  _T("cdsin cdsqrt ceiling cexp char clog cmplx conjg cos cosd cosh count cpu_time cshift csin csqrt dabs ")
		  _T("dacos dacosd dasin dasind datan datan2 datan2d datand date date_and_time dble dcmplx dconjg dcos ")
		  _T("dcosd dcosh dcotan ddim dexp dfloat dflotk dfloti dflotj digits dim dimag dint dlog dlog10 dmax1 ")
		  _T("dmin1 dmod dnint dot_product dprod dreal dsign dsin dsind dsinh dsqrt dtan dtand dtanh eoshift ")
		  _T("epsilon errsns exp exponent float floati floatj floatk floor fraction free huge iabs iachar iand ")
		  _T("ibclr ibits ibset ichar idate idim idint idnint ieor ifix iiabs iiand iibclr iibits iibset iidim ")
		  _T("iidint iidnnt iieor iifix iint iior iiqint iiqnnt iishft iishftc iisign ilen imax0 imax1 imin0 imin1 ")
		  _T("imod index inint inot int int1 int2 int4 int8 iqint iqnint ior ishft ishftc isign isnan izext jiand ")
		  _T("jibclr jibits jibset jidim jidint jidnnt jieor jifix jint jior jiqint jiqnnt jishft jishftc jisign ")
		  _T("jmax0 jmax1 jmin0 jmin1 jmod jnint jnot jzext kiabs kiand kibclr kibits kibset kidim kidint kidnnt ")
		  _T("kieor kifix kind kint kior kishft kishftc kisign kmax0 kmax1 kmin0 kmin1 kmod knint knot kzext lbound ")
		  _T("leadz len len_trim lenlge lge lgt lle llt log log10 logical lshift malloc matmul max max0 max1 ")
		  _T("maxexponent maxloc maxval merge min min0 min1 minexponent minloc minval mod modulo mvbits nearest ")
		  _T("nint not nworkers number_of_processors pack popcnt poppar precision present product radix random ")
		  _T("random_number random_seed range real repeat reshape rrspacing rshift scale scan secnds selected_int_kind ")
		  _T("selected_real_kind set_exponent shape sign sin sind sinh size sizeof sngl snglq spacing spread sqrt ")
		  _T("sum system_clock tan tand tanh tiny transfer transpose trim ubound unpack verify"),

		  _T("cdabs cdcos cdexp cdlog cdsin cdsqrt cotan cotand dcmplx dconjg dcotan dcotand decode dimag dll_export ")
		  _T("dll_import doublecomplex dreal dvchk encode find flen flush getarg getcharqq getcl getdat getenv gettim ")
		  _T("hfix ibchng identifier imag int1 int2 int4 intc intrup invalop iostat_msg isha ishc ishl jfix lacfar ")
		  _T("locking locnear map nargs nbreak ndperr ndpexc offset ovefl peekcharqq precfill prompt qabs qacos qacosd ")
		  _T("qasin qasind qatan qatand qatan2 qcmplx qconjg qcos qcosd qcosh qdim qexp qext qextd qfloat qimag qlog ")
		  _T("qlog10 qmax1 qmin1 qmod qreal qsign qsin qsind qsinh qsqrt qtan qtand qtanh ran rand randu rewrite ")
		  _T("segment setdat settim system timer undfl unlock union val virtual volatile zabs zcos zexp zlog zsin zsqrt"),
		  
		  _T(""), _T(""), _T(""), _T(""), _T(""), _T("") };


EDITLEXER lexFORTRAN = { SCLEX_FORTRAN, IDS_LEXFORTRAN, /*"Fortran 语言",*/ _T("f;for;f90;f95;f2k"), _T(""), II_MAK, &KeyWords_FORTRAN, {
            { STYLE_DEFAULT, /*"默认",*/ DEFAULTSTYLE },
            { SCE_F_COMMENT, /*"注释",*/ COMMENTSTYLE },
            { SCE_F_NUMBER, /*"数字",*/ SETFG(PURPLE) },
            { SCE_F_STRING1, /*"字串 1",*/ STRINGSTYLE },
            { SCE_F_STRING2, /*"字串 2",*/ STRINGSTYLE },
            { SCE_F_STRINGEOL, /*"行尾字串",*/ STRINGSTYLE },
            { SCE_F_OPERATOR, /*"运算符",*/ SETFG(DARKBLUE) },
            { SCE_F_IDENTIFIER, /*"标识符",*/ DEFAULTSTYLE },
            { SCE_F_WORD, /*"字",*/ SETFG(0x0000FF) },
            { SCE_F_WORD2, /*"字 2",*/ SETFG(0x8000FF) },
            { SCE_F_WORD3, /*"字 3",*/ SETFG(0x0080C0) },
            { SCE_F_PREPROCESSOR, /*"预处理器",*/ SETFG(0x800000) },
            { SCE_F_OPERATOR2, /*"运算符 2",*/ SETFG(0x808080) },
            { SCE_F_LABEL, /*"标记",*/ DEFAULTSTYLE },
            { SCE_F_CONTINUATION, /*"CONTINUATION",*/ SETFG(0x008000) },
            EOSTYLE } };




MyLexerManager::MyLexerManager()
{
	data.reserve(30);

	data.push_back(&lexDefault);
	data.push_back(&lexSub);
	data.push_back(&lexMSDOS);
	data.push_back(&lexHTML);
	data.push_back(&lexXML);
	data.push_back(&lexCSS);
	data.push_back(&lexJS);
	data.push_back(&lexVBS);
	data.push_back(&lexPL);
	data.push_back(&lexAS);
	data.push_back(&lexCPP);
	//data.push_back(&lexH);
	data.push_back(&lexCS);
	data.push_back(&lexRC);
	data.push_back(&lexMAK);
	data.push_back(&lexJAVA);
	data.push_back(&lexVB);
	data.push_back(&lexPAS);
	data.push_back(&lexASM);
	data.push_back(&lexSQL);
	data.push_back(&lexPY);
	data.push_back(&lexNSIS);
	data.push_back(&lexINI);
	data.push_back(&lexREG);
	data.push_back(&lexBAT);
	data.push_back(&lexDIFF);
	data.push_back(&lexRUBY);
	data.push_back(&lexFORTRAN);

	curlexer = &lexDefault;
	//std::sort(data.begin(), data.end(), Less());
}


// Currently used lexer
static EDITLEXER * pLexDefault = pApp->Lexer.GetDefault();




namespace sci {

//************************************
// Method:    ConvertFontSize 判断是相对字体大小还是绝对字体大小
// FullName:  sci::ConvertFontSize
// Access:    public const 
// Returns:   DWORD 真实的字体大小
// Qualifier: bluenet
// Parameter: DWORD size 高 16 位为 0 时为绝对大小，否则为相对大小
//************************************

DWORD ConvertFontSize(DWORD size)
{
	DWORD tmp = pApp->BaseFontSize;
	if (0 == HIWORD(size))
		return size;
	else if ((LONG)size < 0)
		return pApp->BaseFontSize + LOWORD(size);

	return pApp->BaseFontSize + LOWORD(size);
}


void GetCurStyle(STYLE & out, int StyleID)
{
	CopyStyle(out, pLexDefault->Styles[0].Value);
	CopyStyle(out, pApp->Lexer.GetCurLexer()->Styles[0].Value);

	int i = 0;
	EDITSTYLE * pStyle = &pApp->Lexer.GetCurLexer()->Styles[i];
	while (-1 != pStyle->iStyle)
	{
		if (pStyle->iStyle == StyleID) break;
		pStyle++;
	}

	CopyStyle(out, pStyle->Value);
}

void GetSelStyle(STYLE & out, EDITSTYLE * pStyle)
{
	ASSERT(pStyle);
	EDITSTYLE * pStyleLexDefault = pStyle;
	while (STYLE_DEFAULT != pStyleLexDefault->iStyle) pStyleLexDefault--;

	EDITSTYLE * pStyleDefault = &pLexDefault->Styles[0];

	CopyStyle(out, pStyleDefault->Value);
	CopyStyle(out, pStyleLexDefault->Value);
	CopyStyle(out, pStyle->Value);
	out.cbStructSize = sizeof(STYLE);
}

void CopyStyle(STYLE & dest, const STYLE & src)
{
	dest.cbStructSize = sizeof(STYLE);
	if (src.FontName[0])
	{
		_tcscpy(dest.FontName, src.FontName);
		dest.fCharSet = src.fCharSet;
	}
	if (src.iFontSize) dest.iFontSize = src.iFontSize;
	if (src.rForeColor) dest.rForeColor = src.rForeColor;
	if (src.rBackColor) dest.rBackColor = src.rBackColor;
	if (src.fBold) dest.fBold = src.fBold;
	if (src.fItalic) dest.fItalic = src.fItalic;
	if (src.fUnderline) dest.fUnderline = src.fUnderline;
	if (src.sEOLFilled) dest.sEOLFilled = src.sEOLFilled;
	if (src.sCaseSensitive) dest.sCaseSensitive = src.sCaseSensitive;
}


const TCHAR * MakeHTMLColorString(COLORREF color)
{
	color = sci::ConvertColor(color);
	static TCHAR buf[8];
	buf[0] = '#';
	_stprintf(buf+1, _T("%.2X"), (int)GetRValue(color));
	_stprintf(buf+3, _T("%.2X"), (int)GetGValue(color));
	_stprintf(buf+5, _T("%.2X"), (int)GetBValue(color));
	buf[7] = 0;
	return buf;
}

tstring MakeHTMLFontString(const STYLE & style)
{
	tstring out = _T("<span style=\"");
	if (style.iFontSize || style.FontName[0]) {
		out += _T("font: ");
		if (style.iFontSize) {
			out << sci::ConvertFontSize(style.iFontSize);
			out << _T("pt");
		}
		if (style.FontName[0]) {
			out += _T(" ");
			out += style.FontName;
		}
		out += _T("; ");
	}

	out += _T("color: ");
	out += MakeHTMLColorString(sci::ConvertColor(style.rForeColor));
	out += _T("; ");
	if (style.fBold) out += _T("font-weight: bold; ");
	if (style.fItalic) out += _T("font-style: italic; ");
	if (style.fUnderline) out += _T("text-decoration: underline; ");

	out += _T("\">");
	return out;
}

const util::ptstring CreateCPPFilter()
{
	util::ptstring pstr;
	tstring & filter = *pstr;

	api::LoadStringT(filter, lexCPP.NameID);
	util::make_before(filter, '\n');
	filter += _T("\n*.");
	filter += lexCPP.szExtensions;
	filter += _T("\n");

	//filter += util::get_before(tstring(ResStr(lexH.NameID).GetPtr()), '\n');
	//filter += _T("\n*.");
	//filter += lexH.szExtensions;
	//filter += _T("\n");

	filter += ResStr(IDS_FILTER_ALL);
	util::replace(filter, _T(";"), _T(";*."));
	FixMutliString(filter);

	return pstr;
}

const util::ptstring CreateAllFilter()
{
	util::ptstring pstr;
	tstring & filter = *pstr;

	api::LoadStringT(filter, IDS_FILTER_ALL);
	filter += _T("\n");

	FixMutliString(filter);

	return pstr;
}



const util::ptstring CreateSupptoredFilter()
{
	util::ptstring pstr;
	tstring & filter = *pstr;

	api::LoadStringT(filter, IDS_FILTER_SUPPORT);
	filter += _T("\n");
	for (unsigned int j = 0; j < pApp->Lexer.GetSize(); j++)
    {
		filter += _T(";");
		filter += pApp->Lexer[j]->szExtensions;
    }
	filter += _T("\n\n");

	util::replace(filter, _T(";"), _T(";*."));
	FixMutliString(filter);

	return pstr;
}


const util::ptstring CreateFullFilter(bool is_open)
{
	util::ptstring pstr;
	tstring & filter = *pstr;

	if (is_open)
	{
#if defined(UNICODE) || _WIN32_WINNT >= 0x0500
		filter += ResStr(IDS_FILTER_SUPPORT);
		filter += _T("\n*.");
		for (unsigned int i = 0; i < pApp->Lexer.GetSize(); i++)
		{
			filter += pApp->Lexer[i]->szExtensions;
			filter += ';';
		}
		filter[filter.length()-1] = '\n';
#endif
		filter += ResStr(IDS_FILTER_ALL);
	}

	tstring tmp;
	for (unsigned int j = 0; j < pApp->Lexer.GetSize(); j++)
    {
		api::LoadStringT(tmp, pApp->Lexer[j]->NameID);
		tmp.erase(util::min(tmp.find('\n'), tmp.length()));
		filter += tmp;
		filter += _T(" (*.");
		filter += pApp->Lexer[j]->szExtensions;
		filter += _T(")");
		filter += _T("\n*.");
		filter += pApp->Lexer[j]->szExtensions;
		filter += '\n';
    }
	filter += '\n';

	util::replace(filter, _T(";"), _T(";*."));
	FixMutliString(filter);

	return pstr;
}

tstring GetCurrentLexerName()
{
	tstring tmp;
	api::LoadStringT(tmp, pApp->Lexer.GetCurLexer()->NameID);
	util::make_before(tmp, '\n');
	return tmp;
}





} //namespace sci





//=============================================================================
//
//  Style_GetLexerIconId()
//
int Style_GetLexerIconId(EDITLEXER * plex)
{
	TCHAR * p;
	const TCHAR * pszExtensions;
	TCHAR * pszFile;

	SHFILEINFO shfi;

	if (_tcslen(plex->szExtensions))
		pszExtensions = plex->szExtensions;
	else
		pszExtensions = plex->pszDefExt;

	pszFile = (TCHAR *)GlobalAlloc(GPTR, _tcslen(pszExtensions) + _tcslen(_T("*.txt")) + 16);
	_tcscpy(pszFile, _T("*."));
	_tcscat(pszFile, pszExtensions);
	if (p = _tcschr(pszFile, ';'))
		* p = '\0';

	// check for ; at beginning
	if (_tcslen(pszFile) < 3)
		_tcscat(pszFile, _T("txt"));

	SHGetFileInfo(pszFile, FILE_ATTRIBUTE_NORMAL, &shfi, sizeof(SHFILEINFO),
				  SHGFI_SMALLICON | SHGFI_SYSICONINDEX | SHGFI_USEFILEATTRIBUTES);

	GlobalFree(pszFile);

	return (shfi.iIcon);
}






// End of Styles
