#ifndef CHANGE_SRC_TRACKING_CHANGE_TRACKER_H_
#define CHANGE_SRC_TRACKING_CHANGE_TRACKER_H_

#include <mutex>
#include <sstream>
#include <unordered_map>

#include "utility/logger.h"

namespace tracking {

class ChangeTracker {
	public:
		ChangeTracker(utility::Logger*, const std::string&);
		ChangeTracker(utility::Logger*);
		~ChangeTracker();

		bool is_tracked(const std::string&) const;

		bool track(const std::string&);

	private:
		const std::string      diff_cmd_;
		utility::Logger* const logger_;
		std::mutex             write_mutex_;

		std::unordered_map<
			std::string, std::unique_ptr<std::stringstream>
		> children_;

};

}

#endif  // CHANGE_SRC_TRACKING_CHANGE_TRACKER_H_
