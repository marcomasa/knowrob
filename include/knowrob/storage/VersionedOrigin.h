/*
 * This file is part of KnowRob, please consult
 * https://github.com/knowrob/knowrob for license details.
 */

#ifndef KNOWROB_VERSIONED_ORIGIN_H
#define KNOWROB_VERSIONED_ORIGIN_H

namespace knowrob {
	/**
	 * A versioned origin is a pair of an origin and a version.
	 */
	class VersionedOrigin {
	public:
		VersionedOrigin(std::string_view origin, std::string_view version)
				: origin_(origin), version_(version) {}

		/**
		 * Compare two versioned origins.
		 * @param other another versioned origin.
		 * @return true if this versioned origin is less than the other.
		 */
		bool operator<(const VersionedOrigin &other) const {
			return origin_ < other.origin_ || (origin_ == other.origin_ && version_ < other.version_);
		}

		/**
		 * @return the origin of this versioned origin.
		 */
		const auto &value() const { return origin_; }

		/**
		 * @return the version of this versioned origin.
		 */
		const auto &version() const { return version_; }

	protected:
		std::string origin_;
		std::string version_;
	};

	using VersionedOriginPtr = std::shared_ptr<VersionedOrigin>;
}

#endif //KNOWROB_VERSIONED_ORIGIN_H
