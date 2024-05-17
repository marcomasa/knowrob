/*
 * This file is part of KnowRob, please consult
 * https://github.com/knowrob/knowrob for license details.
 */

#ifndef KNOWROB_SWRL_REASONER_H
#define KNOWROB_SWRL_REASONER_H

// KnowRob
#include <knowrob/reasoner/prolog/PrologReasoner.h>

namespace knowrob {

	class SWRLReasoner : public PrologReasoner {
	public:
		static const std::string SWRL_FORMAT;

		SWRLReasoner();

		bool loadSWRLFile(const DataSourcePtr &dataFile);

	protected:
		// Override PrologReasoner
		bool initializeDefaultPackages() override;
	};

} // knowrob

#endif //KNOWROB_SWRL_REASONER_H
