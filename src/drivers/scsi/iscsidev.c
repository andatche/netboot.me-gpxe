/*
 * Copyright (C) 2006 Michael Brown <mbrown@fensystems.co.uk>.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of the
 * License, or any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

#include <stddef.h>
#include <gpxe/iscsi.h>

/** @file
 *
 * iSCSI SCSI device
 *
 */

/**
 * Issue SCSI command via iSCSI device
 *
 * @v scsi		SCSI device
 * @v command		SCSI command
 * @ret rc		Return status code
 */
static int iscsi_command ( struct scsi_device *scsi,
			   struct scsi_command *command ) {
	struct iscsi_device *iscsidev
		= container_of ( scsi, struct iscsi_device, scsi );

	return iscsi_issue ( &iscsidev->iscsi, command );
}

/**
 * Initialise iSCSI device
 *
 * @v iscsidev		iSCSI device
 */
int init_iscsidev ( struct iscsi_device *iscsidev ) {
	iscsidev->scsi.command = iscsi_command;
	iscsidev->scsi.lun = iscsidev->iscsi.lun;
	return init_scsidev ( &iscsidev->scsi );
}