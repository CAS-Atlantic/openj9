
/*******************************************************************************
 * Copyright IBM Corp. and others 1991
 *
 * This program and the accompanying materials are made available under
 * the terms of the Eclipse Public License 2.0 which accompanies this
 * distribution and is available at https://www.eclipse.org/legal/epl-2.0/
 * or the Apache License, Version 2.0 which accompanies this distribution and
 * is available at https://www.apache.org/licenses/LICENSE-2.0.
 *
 * This Source Code may also be made available under the following
 * Secondary Licenses when the conditions for such availability set
 * forth in the Eclipse Public License, v. 2.0 are satisfied: GNU
 * General Public License, version 2 with the GNU Classpath
 * Exception [1] and GNU General Public License, version 2 with the
 * OpenJDK Assembly Exception [2].
 *
 * [1] https://www.gnu.org/software/classpath/license.html
 * [2] https://openjdk.org/legal/assembly-exception.html
 *
 * SPDX-License-Identifier: EPL-2.0 OR Apache-2.0 OR GPL-2.0-only WITH Classpath-exception-2.0 OR GPL-2.0-only WITH OpenJDK-assembly-exception-1.0
 *******************************************************************************/

/**
 * @file
 * @ingroup GC_Structs
 */

#include "j9.h"
#include "j9cfg.h"
#include "j9cp.h"

#include "ConstantPoolClassSlotIterator.hpp"

/**
 * @return the next Class reference from the constant pool
 * @return NULL if there are no more references
 */
J9Class *
GC_ConstantPoolClassSlotIterator::nextSlot()
{
	J9Class *classPtr = NULL;
	while (_cpEntryCount) {
		if (0 == _cpDescriptionIndex) {
			_cpDescription = *_cpDescriptionSlots;
			_cpDescriptionSlots += 1;
			_cpDescriptionIndex = J9_CP_DESCRIPTIONS_PER_U32;
		}

		uint32_t slotType = _cpDescription & J9_CP_DESCRIPTION_MASK;
		J9Object **slotPtr = _cpEntry;

		/* Adjust the CP slot and description information */
		_cpEntry = (J9Object **)(((uint8_t *)_cpEntry) + sizeof(J9RAMConstantPoolItem));
		_cpEntryCount -= 1;

		_cpDescription >>= J9_CP_BITS_PER_DESCRIPTION;
		_cpDescriptionIndex -= 1;

		/* Determine if the slot should be processed */
		if (slotType == J9CPTYPE_CLASS) {
			classPtr = ((J9RAMClassRef *) slotPtr)->value;
			if (NULL != classPtr) {
				break;
			}
		}
	}
	return classPtr;
}
