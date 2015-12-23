#ifndef CHANGE_SRC_CHANGE_TRACKER_H_
#define CHANGE_SRC_CHANGE_TRACKER_H_

#include "utility.h"

#include <unordered_map>

#include <boost/process.hpp>

namespace utility {

class ChangeTracker {
	public:
		ChangeTracker(utility::Logger*);
		~ChangeTracker();

		bool is_tracked(const std::string&) const;

		bool track(const std::string&);

	private:
		utility::Logger* const logger_;

		std::unordered_map<
			std::string, std::unique_ptr<boost::process::child>
		> children_;

};

}

#endif  // CHANGE_SRC_CHANGE_TRACKER_H_
