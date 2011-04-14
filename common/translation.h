/* Residual - A 3D game interpreter
 *
 * Residual is the legal property of its developers, whose names
 * are too numerous to list here. Please refer to the AUTHORS
 * file distributed with this source distribution.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 *
 * $URL$
 * $Id$
 */

#ifndef COMMON_TRANSLATION_H
#define COMMON_TRANSLATION_H

#include "common/singleton.h"
#include "common/str-array.h"

#ifdef USE_TRANSLATION

namespace Common {

class File;
class FSNode;

enum TranslationIDs {
	kTranslationAutodetectId = 0,
	kTranslationBuiltinId = 1000
};

struct TLanguage {
	const char *name;
	int id;

	TLanguage() : name(0), id(0) {}
	TLanguage(const char *n, int i) : name(n), id(i) {}
};

bool operator<(const TLanguage &l, const TLanguage &r);

typedef Array<TLanguage> TLangArray;

struct PoMessageEntry {
	int msgid;
	String msgctxt;
	String msgstr;
};

/**
 * Message translation manager.
 */
class TranslationManager : public Singleton<TranslationManager> {
public:
	/**
	 * The constructor detects the system language and sets default
	 * language to English.
	 */
	TranslationManager();
	~TranslationManager();

	/**
	 * Retrieves the language string to the given id.
	 *
	 * @param id Id of the language
	 * @return the matching string description of the language
	 */
	String getLangById(int id) const;

	/**
	 * Sets the current translation language to the one specified in the
	 * parameter. If the parameter is an empty string, it sets the default
	 * system language.
	 *
	 * @param lang Language to setup.
	 */
	void setLanguage(const String &lang);

	/**
	 * Sets the current translation language to the one specified by the
	 * id parameter.
	 *
	 * @param id The id of the language.
	 */
	void setLanguage(int id) {
		setLanguage(getLangById(id));
	}

	/**
	 * Parses a language string and returns an id instead.
	 *
	 * @param lang Language string
	 * @return id of the language or kTranslationBuiltinId in case the
	 *         language could not be found.
	 */
	int parseLanguage(const String &lang) const;

	/**
	 * Returns the translation into the current language of the parameter
	 * message. In case the message isn't found in the translation catalog,
	 * it returns the original untranslated message.
	 */
	const char *getTranslation(const char *message) const;

	/**
	 * Returns the translation into the current language of the parameter
	 * message. In case the message isn't found in the translation catalog,
	 * it returns the original untranslated message.
	 */
	String getTranslation(const String &message) const;

	/**
	 * Returns the translation into the current language of the parameter
	 * message. In case the message isn't found in the translation catalog,
	 * it returns the original untranslated message.
	 *
	 * If a translation is found for the given context it will return that
	 * translation, otherwise it will look for a translation for the same
	 * massage without a context or with a different context.
	 */
	const char *getTranslation(const char *message, const char *context) const;

	/**
	 * Returns the translation into the current language of the parameter
	 * message. In case the message isn't found in the translation catalog,
	 * it returns the original untranslated message.
	 *
	 * If a translation is found for the given context it will return that
	 * translation, otherwise it will look for a translation for the same
	 * massage without a context or with a different context.
	 */
	String getTranslation(const String &message, const String &context) const;

	/**
	 * Returns a list of supported languages.
	 *
	 * @return The list of supported languages in a user readable form.
	 */
	const TLangArray getSupportedLanguageNames() const;

	/**
	 * Returns charset specified by selected translation language
	 */
	String getCurrentCharset() const;

	/**
	 * Returns currently selected translation language
	 */
	String getCurrentLanguage() const;

private:
	/**
	 * Tries to find the given language or a derivate of it.
	 *
	 * @param lang Language string
	 * @return id of the language or -1 in case no matching language could
	 *         be found.
	 */
	int32 findMatchingLanguage(const String &lang);

	/**
	 * Find the translations.dat file. It looks first using the SearchMan and
	 * then if needed using the Themepath. If found it opens the given File
	 * to read the translations.dat file.
	 */
	bool openTranslationsFile(File&);

	/**
	 * Find the translations.dat file in the given directory node.
	 * If found it opens the given File to read the translations.dat file.
	 */
	bool openTranslationsFile(const FSNode &node, File&, int depth = -1);

	/**
	 * Load the list of languages from the translations.dat file
	 */
	void loadTranslationsInfoDat();

	/**
	 * Load the translation for the given language from the translations.dat file
	 *
	 * @param index of the language in the list of languages
	 */
	void loadLanguageDat(int index);

	/**
	 * Check the header of the given file to make sure it is a valid translations data file.
	 */
	bool checkHeader(File &in);

	StringArray _langs;
	StringArray _langNames;

	StringArray _messageIds;
	Array<PoMessageEntry> _currentTranslationMessages;
	String _currentCharset;
	int _currentLang;
};

} // End of namespace Common

#define TransMan Common::TranslationManager::instance()

#define _(str) TransMan.getTranslation(str)
#define _c(str, context) TransMan.getTranslation(str, context)

#else // !USE_TRANSLATION

#define _(str) str
#define _c(str, context) str

#endif // USE_TRANSLATION

#define _s(str) str
#define _sc(str, ctxt) str
#define DECLARE_TRANSLATION_ADDITIONAL_CONTEXT(str, ctxt)

#endif // COMMON_TRANSLATION_H