/**
 * @file
 */
#pragma once

namespace birch {
/**
 * Does this operator exist in C++?
 */
bool isTranslatable(const std::string& op);

/**
 * Generate nice name. This replaces operators with words.
 */
std::string nice(const std::string& name);

/**
 * Generate C++ name. This is the original name, with any characters
 * outside the class [0-9a-zA-z_] translated to within that class, and an
 * underscore added to the end to avoid clashes with user variables.
 */
std::string sanitize(const std::string& name);

/**
 * Escape unicode characters in a string.
 */
std::string escape_unicode(const std::string& str);

/**
 * Process a documentation comment to detailed description.
 */
std::string detailed(const std::string& str);

/**
 * Process a documentation comment to brief description.
 */
std::string brief(const std::string& str);

/**
 * Process a documentation comment to a single line.
 */
std::string one_line(const std::string& str);

/**
 * Process a string into an anchor name for Markdown.
 */
std::string anchor(const std::string& str);

/**
 * Quote lines in a string with the given prefix.
 */
std::string quote(const std::string& str, const std::string& indent);

/**
 * Convert a string to lower case.
 */
std::string lower(const std::string& str);

/**
 * Convert a string to upper case.
 */
std::string upper(const std::string& str);

/**
 * Tar name for a package.
 */
std::string tar(const std::string& name);

/**
 * Canonical name for a package. This is the same as the tar name, with
 * hyphens replaced with underscores.
 */
std::string canonical(const std::string& name);

}
