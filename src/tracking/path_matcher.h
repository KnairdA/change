#ifndef CHANGE_SRC_TRACKING_PATH_MATCHER_H_
#define CHANGE_SRC_TRACKING_PATH_MATCHER_H_

#include <regex>
#include <vector>

namespace tracking {

class PathMatcher {
	public:
		PathMatcher() = default;
		PathMatcher(const std::string&);

		bool isMatching(const std::string&) const;

	private:
		std::vector<std::regex> patterns_;
};

}

#endif  // CHANGE_SRC_TRACKING_PATH_MATCHER_H_
