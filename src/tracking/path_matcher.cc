#include "path_matcher.h"

#include <algorithm>

#include <boost/filesystem.hpp>
#include <boost/filesystem/fstream.hpp>

namespace tracking {

PathMatcher::PathMatcher(const std::string& source_file_path) {
	try {
		boost::filesystem::ifstream file(source_file_path);

		if ( file.is_open() ) {
			std::string current_line;

			while ( std::getline(file, current_line) ) {
				if ( current_line[0] != '#' ) { // i.e. not a comment
					try {
						this->patterns_.emplace_back(current_line);
					} catch ( const std::regex_error& ) { }
				}
			}
		}
	} catch ( boost::filesystem::filesystem_error& ) {
		// invalid source path is not relevant as we can easily fall back to declining
		// all candidate paths.
	}
}

bool PathMatcher::isMatching(const std::string& candidate) const {
	return std::any_of(
		this->patterns_.begin(),
		this->patterns_.end(),
		[&candidate](const std::regex& pattern) -> bool {
			return std::regex_match(candidate, pattern);
		}
	);
}

}
