/****************************************************************************
 *
 * Copyright 2016 Samsung Electronics All Rights Reserved.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing,
 * software distributed under the License is distributed on an
 * "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND,
 * either express or implied. See the License for the specific
 * language governing permissions and limitations under the License.
 *
 ****************************************************************************/
/****************************************************************************
 * fs/vfs/fs_close.c
 *
 *   Copyright (C) 2007-2009, 2012 Gregory Nutt. All rights reserved.
 *   Author: Gregory Nutt <gnutt@nuttx.org>
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in
 *    the documentation and/or other materials provided with the
 *    distribution.
 * 3. Neither the name NuttX nor the names of its contributors may be
 *    used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
 * FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
 * COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
 * BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS
 * OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED
 * AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN
 * ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 *
 ****************************************************************************/

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <tinyara/config.h>

#include <unistd.h>
#include <sched.h>
#include <errno.h>
#include <tinyara/fs/fs.h>
#include <tinyara/cancelpt.h>

#if defined(CONFIG_NET) && CONFIG_NSOCKET_DESCRIPTORS > 0
#include <tinyara/net/net.h>
#endif

#include "inode/inode.h"

/****************************************************************************
 * Global Functions
 ****************************************************************************/

/****************************************************************************
 * Name: close
 *
 * Description:
 *   close() closes a file descriptor, so that it no longer refers to any
 *   file and may be reused. Any record locks (see fcntl(2)) held on the file
 *   it was associated with, and owned by the process, are removed (regardless
 *   of the file descriptor that was used to obtain the lock).
 *
 *   If fd is the last copy of a particular file descriptor the resources
 *   associated with it are freed; if the descriptor was the last reference
 *   to a file which has been removed using unlink(2) the file is deleted.
 *
 * Parameters:
 *   fd   file descriptor to close
 *
 * Returned Value:
 *   0 on success; -1 on error with errno set appropriately.
 *
 * Assumptions:
 *
 ****************************************************************************/

int close(int fd)
{
	int err;
#if (CONFIG_NFILE_DESCRIPTORS > 0) || (CONFIG_NSOCKET_DESCRIPTORS > 0)
	int ret;
#endif
	/* close() is a cancellation point */
	(void)enter_cancellation_point();

#if CONFIG_NFILE_DESCRIPTORS > 0
	/* Did we get a valid file descriptor? */

	if ((unsigned int)fd >= CONFIG_NFILE_DESCRIPTORS)
#endif
	{
		/* Close a socket descriptor */

#if defined(CONFIG_NET) && CONFIG_NSOCKET_DESCRIPTORS > 0
		if ((unsigned int)fd < (CONFIG_NFILE_DESCRIPTORS + CONFIG_NSOCKET_DESCRIPTORS)) {
			ret = net_close(fd);
			leave_cancellation_point();
			return ret;
		} else
#endif
		{
			err = EBADF;
			goto errout;
		}
	}
#if CONFIG_NFILE_DESCRIPTORS > 0
	/* Close the driver or mountpoint.  NOTES: (1) there is no
	 * exclusion mechanism here , the driver or mountpoint must be
	 * able to handle concurrent operations internally, (2) The driver
	 * may have been opened numerous times (for different file
	 * descriptors) and must also handle being closed numerous times.
	 * (3) for the case of the mountpoint, we depend on the close
	 * methods bing identical in signature and position in the operations
	 * vtable.
	 */

	ret = files_close(fd);
	if (ret < 0) {
		/* An error occurred while closing the driver */

		err = -ret;
		goto errout;
	}
	leave_cancellation_point();
	return OK;

#endif

errout:
	set_errno(err);
	leave_cancellation_point();
	return ERROR;
}
