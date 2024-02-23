/*
 * This file is part of KnowRob, please consult
 * https://github.com/knowrob/knowrob for license details.
 */

#ifndef KNOWROB_XSD_ATOMIC_H
#define KNOWROB_XSD_ATOMIC_H

#include "string_view"
#include "Atomic.h"
#include "RDFNode.h"
#include "XSDType.h"

namespace knowrob {
	/**
	 * Get the XSD type from the IRI
	 * @param iri the IRI of the XSD type
	 * @return the XSD type
	 */
	XSDType xsdTypeFromIRI(std::string_view iri);

	/**
	 * Get the IRI of the XSD type
	 * @param type the XSD type
	 * @return the IRI of the XSD type
	 */
	std::string_view xsdTypeToIRI(XSDType type);

	/**
	 * An atomic RDF node with an XSD datatype
	 */
	class XSDAtomic : public RDFNode, public Atomic {
	public:
		XSDAtomic() : RDFNode(), Atomic() {}

		/**
		 * The IRI of the XSD datatype
		 * @return the IRI of the XSD datatype of the RDF literal
		 */
		std::string_view xsdTypeIRI() const;

		/**
		 * The XSD datatype determines how the lexical form maps to a literal value
		 * @return the XSD datatype of the RDF literal
		 */
		virtual XSDType xsdType() const = 0;

		// override RDFNode
		RDFNodeType rdfNodeType() const override { return RDFNodeType::LITERAL; }
	};

} // knowrob

#endif //KNOWROB_XSD_ATOMIC_H