/*
** hostcachehost.cpp
**
** Copyright © Quazaa Development Team, 2009-2013.
** This file is part of QUAZAA (quazaa.sourceforge.net)
**
** Quazaa is free software; this file may be used under the terms of the GNU
** General Public License version 3.0 or later as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL included in the
** packaging of this file.
**
** Quazaa is distributed in the hope that it will be useful,
** but WITHOUT ANY WARRANTY; without even the implied warranty of
** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
**
** Please review the following information to ensure the GNU General Public
** License version 3.0 requirements will be met:
** http://www.gnu.org/copyleft/gpl.html.
**
** You should have received a copy of the GNU General Public License version
** 3.0 along with Quazaa; if not, write to the Free Software Foundation,
** Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*/

#include "hostcachehost.h"

template<>
class qLess <CHostCacheHost*>
{
public:
	inline bool operator()(const CHostCacheHost* l, const CHostCacheHost* r) const
	{
		return l->m_tTimestamp > r->m_tTimestamp;
	}
};

CHostCacheHost::CHostCacheHost(CEndPoint oAddress, quint32 tTimestamp) :
	m_oAddress( oAddress ),
	m_tTimestamp( tTimestamp ),
	m_nQueryKey(    0 ),
	m_nKeyHost( CEndPoint() ),
	m_nKeyTime(     0 ),
	m_tAck(         0 ),
	m_tLastQuery(   0 ),
	m_tRetryAfter(  0 ),
	m_tLastConnect( 0 ),
	m_nFailures(    0 )
{
}

CHostCacheHost::~CHostCacheHost()
{
}

bool CHostCacheHost::canQuery(const quint32 tNow)
{
	Q_ASSERT( tNow );

	if ( m_tAck && m_nQueryKey ) // if waiting for an ack, and we have a query key
	{
		return false;
	}

	Q_ASSERT( tNow - m_tTimestamp >= 0 );
	if ( tNow - m_tTimestamp > quazaaSettings.Gnutella2.HostCurrent ) // host is not too old
	{
		return false;
	}

	if ( m_tRetryAfter && tNow < m_tRetryAfter ) // honor retry-after
	{
		return false;
	}

	if ( !m_tLastQuery ) // host not already queried
	{
		return true;
	}

	Q_ASSERT( tNow - m_tLastQuery > 0 );
	return tNow - m_tLastQuery > quazaaSettings.Gnutella2.QueryHostThrottle;
}

void CHostCacheHost::setKey(quint32 nKey, const quint32 tNow, CEndPoint* pHost)
{
	m_tAck      = 0;
	m_nFailures = 0;
	m_nQueryKey = nKey;
	m_nKeyTime  = tNow;
	m_nKeyHost  = pHost ? *pHost : Network.getLocalAddress();
}
