#ifndef USERAGENTRULE_H
#define USERAGENTRULE_H

#include "securerule.h"

class CUserAgentRule : public CSecureRule
{
private:
	bool				m_bRegExp;  // is the content of this rule is a regular expression?
	QRegularExpression	m_regularExpressionContent;

public:
	CUserAgentRule();

	bool				operator==(const CSecureRule& pRule) const;

	void				setRegExp(bool bRegExp);
	bool			getRegExp() const;

	bool				parseContent(const QString& sContent);

	CSecureRule*	getCopy() const;

	bool				match(const QString& sUserAgent) const;
	void				toXML(QXmlStreamWriter& oXMLdocument) const;
};

#endif // USERAGENTRULE_H
