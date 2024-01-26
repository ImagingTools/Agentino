#pragma once


// Qt includes
#include <QtCore/QProcess>


namespace agentinodata
{


struct ProcessStateEnum
{
	QByteArray id;
	QString name;
};

ProcessStateEnum GetProcceStateRepresentation(QProcess::ProcessState processState);



} // namespace agentinodata



