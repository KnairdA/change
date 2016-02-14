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

		// Begin tracking changes to a given path if not already covered.
		void track(const std::string&);

	private:
		const std::string      diff_cmd_;
		utility::Logger* const logger_;
		std::mutex             write_mutex_;

		std::unordered_map<
			std::string,
			std::unique_ptr<std::stringstream>
		> children_;

		bool is_tracked(const boost::filesystem::path&) const;

		// threadsafe child emplacement
		auto create_child(const boost::filesystem::path&);

};

}

#endif  // CHANGE_SRC_TRACKING_CHANGE_TRACKER_H_
