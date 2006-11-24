// Scintilla source code edit control
/** @file AutoComplete.h
 ** Defines the auto completion list box.
 **/
// Copyright 1998-2003 by Neil Hodgson <neilh@scintilla.org>
// The License.txt file describes the conditions under which this software may be distributed.

namespace sci {

#ifndef AUTOCOMPLETE_H
#define AUTOCOMPLETE_H

/**
 */
class AutoComplete {
	bool active;
	tchar stopChars[256];
	tchar fillUpChars[256];
	tchar separator;
	tchar typesep; // Type seperator

public:
	bool ignoreCase;
	bool chooseSingle;
	ListBox *lb;
	int posStart;
	int startLen;
	/// Should autocompletion be canceled if editor's currentPos <= startPos?
	bool cancelAtStartPos;
	bool autoHide;
	bool dropRestOfWord;

	AutoComplete();
	~AutoComplete();

	/// Is the auto completion list displayed?
	bool Active();

	/// Display the auto completion list positioned to be near a character position
	void Start(Window &parent, int ctrlID, int position, Point location,
		int startLen_, int lineHeight, bool unicodeMode);

	/// The stop chars are characters which, when typed, cause the auto completion list to disappear
	void SetStopChars(const tchar *stopChars_);
	bool IsStopChar(tchar ch);

	/// The fillup chars are characters which, when typed, fill up the selected word
	void SetFillUpChars(const tchar *fillUpChars_);
	bool IsFillUpChar(tchar ch);

	/// The separator character is used when interpreting the list in SetList
	void SetSeparator(tchar separator_);
	tchar GetSeparator();

	/// The typesep character is used for seperating the word from the type
	void SetTypesep(tchar separator_);
	tchar GetTypesep();

	/// The list string contains a sequence of words separated by the separator character
	void SetList(const tchar *list);

	void Show(bool show);
	void Cancel();

	/// Move the current list element by delta, scrolling appropriately
	void Move(int delta);

	/// Select a list element that starts with word as the current element
	void Select(const tchar *word);
};

#endif

}; //namespace sci
