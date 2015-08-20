
/*
 * Server for hamlib's TCP rigctl protocol.
 * Copyright (C) 2010 Adam Sampson <ats@offog.org>
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation, either version 2 of the
 * License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

/*
 *
 * Modified for QtRadio by Glenn VE9GJ Sept 29 2011
 * Taken from sdr-shell project
 *
 * Applied remote set filter function over hamlib port
 * by Oliver Goldenstein, DL6KBG, Nov 16 2012
 *
 */
#include <cstdio>
#include <unistd.h>

#include "rigctl.h"
#include "rigctl.moc"
#include <string>
#include <vector>

RigCtlSocket::RigCtlSocket(QObject *parent, RigCtl *rig, QTcpSocket *conn)
        : QObject(parent) {
	this->conn = conn;
	this->m_rig = rig;
}

void RigCtlSocket::disconnected() {
        deleteLater();
}

void RigCtlSocket::readyRead() {
        if (!conn->canReadLine()) {
                return;
        }

        QByteArray command(conn->readLine());
        // removed command.chop(1); because it is unable to cope with
        // with more than one character line terminations
        // i.e. telnet client uses a CR/LF sequence
        command = command.simplified();
        if (command.size() == 0) {
                command.append("?");
        }

        QString cmdstr = command.constData();
        QStringList cmdlist = cmdstr.split(QRegExp("\\s+"));
        int cmdlistcnt = cmdlist.count();
        bool output = false;
        int retcode = 0; //RIG_OK
        QTextStream out(conn);

        /* This isn't a full implementation of the rigctl protocol; it's
           essentially just enough to keep fldigi happy.  (And not very happy
           at that; you will need to up the delays to stop it sending a
           command, ignoring the reply owing to a timeout, then getting
           confused the next time it sends a command because the old reply is
           already in the buffer.)

           Not implemented but used by fldigi:
             v           get_vfo  -fixed
             F 1.234     set_freq -fixed
         */

        if (command[0] == 'f') { // get_freq
            out << m_rig->getFreq() << "\n";
            output = true;
        } else if(cmdlist[0].compare("F") == 0 && cmdlistcnt == 2) { // set_freq
            QString newf = cmdlist[1];
            m_rig->setFreq(atol(newf.toUtf8()));
        } else if (command[0] == 'm') { // get_mode
            //output = true;
        } else if (command[0] == 'v') { // get_vfo
            //output = true;
        } else if (command[0] == 'V') { // set_VFO
            QString cmd = command.constData();
            if ( cmd.contains("VFOA")){
               //main->rigctlSetVFOA();
            }
            if ( cmd.contains("VFOB")){
               //main->rigctlSetVFOB();
            }
        } else if (command[0] == 'j') { // get_rit
            out << "0" << "\n";
            output = true;
        } else if (command[0] == 's') { // get_split_vfo
            // simple "we don't do split" response

            // TODO - if split is selected then VFOS will be returned
            // which is invalid. This needs to be '1\n<Tx-VFO>' if split is
            // enabled and the Tx-VFO probably needs to be VFOB
            out << "0" << "\n";
            // if split is to be supported then this needs to be the
            // Tx VFO and other split functions will probably need to
            // be implemented
            output = true;
	} else if (command[0] == '\\' || command[0] == '1') {
            // See dump_state in rigctl_parse.c for what this means.
            out << "0\n"; // protocol version
            out << "2" << "\n"; //RIG_MODEL_NETRIGCTL
            out << "1" << "\n"; //RIG_ITU_REGION1
            // Not sure exactly what to send here but this seems to work
            out << "150000.000000 30000000.000000  0x900af -1 -1 0x10000003 0x3\n"; //("%"FREQFMT" %"FREQFMT" 0x%x %d %d 0x%x 0x%x\n",start,end,modes,low_power,high_power,vfo,ant)
            out << "0 0 0 0 0 0 0\n";
            out << "150000.000000 30000000.000000  0x900af -1 -1 0x10000003 0x3\n"; //TX
            out << "0 0 0 0 0 0 0\n";
            out << "0 0\n";
            out << "0 0\n";
            out << "0\n";
            out << "0\n";
            out << "0\n";
            out << "0\n";
            out << "\n";
            out << "\n";
            out << "0x0\n";
            out << "0x0\n";
            out << "0x0\n";
            out << "0x0\n";
            out << "0x0\n";
            out << "0\n";
            output = true;
        } else {
        //  fprintf(stderr, "rigctl: unknown command \"%s\"\n", command.constData());
            retcode = -11;
        }
        // fprintf(stderr, "rigctl:  command \"%s\"\n", command.constData());
        if (!output) {
                out << "RPRT " << retcode << "\n";
        }
}

const unsigned short RigCtlServer::RIGCTL_PORT(19999);

double RigCtl::getFreq() {
	return m_freq;
}

double RigCtl::setFreq(double freq) {
	m_freq = freq;
	return m_freq;
}

RigCtl* RigCtlServer::getRig() {
	return m_rig;
}

RigCtlServer::RigCtlServer(QObject *parent,  unsigned short rigctl_port)
        : QObject(parent) {
	m_rig = new RigCtl();
        server = new QTcpServer(this);
        if (!server->listen(QHostAddress::Any, rigctl_port)) {
                fprintf(stderr, "rigctl: failed to bind socket on port %d\n", rigctl_port);
                return;
        }
        fprintf(stderr, "rigctl: Listening on port %d\n", rigctl_port);
        connect(server, SIGNAL(newConnection()), this, SLOT(newConnection()));
}

void RigCtlServer::newConnection() {
	printf ("Rig Connected.\n");
	QTcpSocket *conn = server->nextPendingConnection();
        RigCtlSocket *sock = new RigCtlSocket(this, m_rig, conn);
        connect(conn, SIGNAL(disconnected()), conn, SLOT(deleteLater()));
        connect(conn, SIGNAL(disconnected()), sock, SLOT(disconnected()));
        connect(conn, SIGNAL(readyRead()), sock, SLOT(readyRead()));
}

#include "QtCore/qcoreapplication.h"
int main(int argc, char *argv[])
{
	QCoreApplication app(argc, argv);
	QObject *object;
	RigCtl *rig;
	RigCtlServer *server;
	double lastfreq;

	object = new QObject();
	server = new RigCtlServer(object, 19997);
	rig = server->getRig();
	rig->setFreq(434.5e6);
	lastfreq = 0;

	while (true) {
		usleep(1e5);
		app.processEvents();
		double newfreq = rig->getFreq();
		if (newfreq != lastfreq) {
			printf ("Rig Freq: %fMHz\n", newfreq * 1.0e-6);
			lastfreq = newfreq;
		}
	}
	return 0;
}

