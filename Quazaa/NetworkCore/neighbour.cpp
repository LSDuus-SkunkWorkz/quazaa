/*
** $Id$
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

#include "neighbour.h"
#include "neighbours.h"
#include "quazaasettings.h"
#include "hostcache.h"
#include <QTcpSocket>
#include "securitymanager.h"

#include "debug_new.h"

CNeighbour::CNeighbour(QObject* parent) :
	CCompressedConnection(parent)
{
	m_nProtocol = dpNull;

	m_nState = nsClosed;
	m_tLastPacketIn = m_tLastPacketOut = 0;
	m_tLastPingOut  = 0;
	m_nPingsWaiting = 0;
	m_tRTT = 0;
	m_nPacketsIn = m_nPacketsOut = 0;
	m_bAutomatic = true;

}

CNeighbour::~CNeighbour()
{
	ASSUME_LOCK(Neighbours.m_pSection);
	Neighbours.removeNode(this);
}

void CNeighbour::onTimer(quint32 tNow)
{
	if ( m_nState < nsConnected )
	{
		if ( tNow - m_tConnected > quazaaSettings.Connection.TimeoutConnect )
		{
			if ( m_bInitiated )
			{
				hostCache.m_pSection.lock();
				hostCache.onFailure( m_oAddress );
				hostCache.m_pSection.unlock();
			}

			if ( m_nState == nsConnecting )
				systemLog.postLog( LogSeverity::Information,  Components::Network,
								   qPrintable( tr( "Timed out connecting to %s." ) ),
								   qPrintable( m_oAddress.toStringWithPort() ) );
			else
				systemLog.postLog( LogSeverity::Information,  Components::Network,
								   qPrintable( tr( "Timed out handshaking with %s." ) ),
								   qPrintable( m_oAddress.toStringWithPort() ) );

			close();
			return;
		}
	}
	else if(m_nState == nsConnected)
	{
		if(tNow - m_tLastPacketIn > quazaaSettings.Connection.TimeoutTraffic)
		{
			systemLog.postLog(LogSeverity::Error, tr("Closing connection to %1 due to lack of traffic.").arg(m_oAddress.toString()));
			systemLog.postLog(LogSeverity::Debug, QString("Conn %1, Packet %2, bytes avail %3, net bytes avail %4, ping %5").arg(tNow - m_tConnected).arg(tNow - m_tLastPacketIn).arg(bytesAvailable()).arg(networkBytesAvailable()).arg(tNow - m_tLastPingOut));
			close();
			return;
		}

		if(m_nPingsWaiting > 0 && tNow - m_tLastPingOut > quazaaSettings.Gnutella2.PingTimeout && tNow - m_tLastPacketIn > quazaaSettings.Connection.TimeoutTraffic)
		{
			systemLog.postLog(LogSeverity::Debug, QString("Closing connection with %1 ping timed out").arg(m_oAddress.toString()));
			close();
			return;
		}
	}
}

void CNeighbour::close(bool bDelayed)
{
	m_nState = nsClosing;
	CCompressedConnection::close(bDelayed);
}

void CNeighbour::onDisconnectNode()
{
	Neighbours.m_pSection.lock();
	delete this;
	Neighbours.m_pSection.unlock();
}

void CNeighbour::onError(QAbstractSocket::SocketError e)
{
	if ( e == QAbstractSocket::RemoteHostClosedError )
	{
		if ( m_nState != nsHandshaking )
			systemLog.postLog( LogSeverity::Information, Components::Network,
							   QString("Neighbour %1 dropped connection unexpectedly.")
							   .arg( m_oAddress.toString().toLocal8Bit().constData() ) );
		else
		{
			systemLog.postLog( LogSeverity::Information, Components::Network,
							   QString("Neighbour %1 dropped connection during handshake.")
							   .arg( m_oAddress.toString().toLocal8Bit().constData() ) );

			if ( m_bInitiated )
			{
				hostCache.m_pSection.lock();
				hostCache.remove( m_oAddress );
				hostCache.m_pSection.unlock();

				// for some bad clients that drop connections too early
				securityManager.ban( m_oAddress, RuleTime::FiveMinutes,
									 true, "Dropped handshake" );
			}
		}
	}
	else
	{
		systemLog.postLog( LogSeverity::Error, Components::Network,
						   "Neighbour %s dropped connection unexpectedly (socket error: %s).",
						   qPrintable( m_oAddress.toStringWithPort() ),
						   qPrintable( m_pSocket->errorString() ) );

		if ( m_bInitiated )
		{
			hostCache.m_pSection.lock();
			hostCache.onFailure( m_oAddress );
			hostCache.m_pSection.unlock();
		}
	}

	Neighbours.m_pSection.lock();
	delete this;
	Neighbours.m_pSection.unlock();
}

