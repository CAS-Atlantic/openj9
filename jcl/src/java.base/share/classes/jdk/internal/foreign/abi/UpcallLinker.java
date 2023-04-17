/*[INCLUDE-IF JAVA_SPEC_VERSION >= 19]*/
/*******************************************************************************
 * Copyright IBM Corp. and others 2021
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
 * SPDX-License-Identifier: EPL-2.0 OR Apache-2.0 OR GPL-2.0 WITH Classpath-exception-2.0 OR LicenseRef-GPL-2.0 WITH Assembly-exception
 *******************************************************************************/
package jdk.internal.foreign.abi;

import java.lang.invoke.MethodHandle;
import java.lang.invoke.MethodType;

import java.lang.foreign.FunctionDescriptor;
import java.lang.foreign.MemorySegment;
/*[IF JAVA_SPEC_VERSION >= 20]*/
import java.lang.foreign.SegmentScope;
/*[ELSE] JAVA_SPEC_VERSION >= 20 */
import java.lang.foreign.MemorySession;
/*[ENDIF] JAVA_SPEC_VERSION >= 20 */
/*[IF JAVA_SPEC_VERSION >= 21]*/
import jdk.internal.foreign.abi.AbstractLinker.UpcallStubFactory;
/*[ENDIF] JAVA_SPEC_VERSION >= 21 */
import openj9.internal.foreign.abi.InternalUpcallHandler;

/**
 * The counterpart in OpenJDK is replaced with this class that wrap up
 * an upcall handler enabling the native call to the java code at runtime.
 */
public final class UpcallLinker {

	private final long thunkAddr;

	/* The constructor creates an upcall handler specific to the requested java method
	 * by generating a native thunk in upcall on a given platform.
	 */
	/*[IF JAVA_SPEC_VERSION >= 20]*/
	UpcallLinker(MethodHandle target, MethodType methodType, FunctionDescriptor descriptor, SegmentScope session)
	/*[ELSE] JAVA_SPEC_VERSION >= 20 */
	UpcallLinker(MethodHandle target, MethodType methodType, FunctionDescriptor descriptor, MemorySession session)
	/*[ENDIF] JAVA_SPEC_VERSION >= 20 */
	{
		InternalUpcallHandler internalUpcallHandler = new InternalUpcallHandler(target, methodType, descriptor, session);
		/* The thunk address must be set given entryPoint() is used in OpenJDK. */
		thunkAddr = internalUpcallHandler.upcallThunkAddr();
	}

	/**
	 * Returns the address of the generated thunk at runtime.
	 *
	 * @return the thunk address
	 */
	public long entryPoint() {
		return thunkAddr;
	}

	/*[IF JAVA_SPEC_VERSION >= 20]*/
	/**
	 * The method invoked via Clinker generates a native thunk to create
	 * a native symbol that holds an entry point to the native function
	 * intended for the requested java method in upcall.
	 *
	 * @param target the upcall method handle to the requested java method
	 * @param methodType the MethodType of the upcall method handle
	 * @param descriptor the FunctionDescriptor of the upcall method handle
	 * @param session the SegmentScope of the upcall method handle
	 * @return the native symbol
	 */
	public static MemorySegment make(MethodHandle target, MethodType methodType, FunctionDescriptor descriptor, SegmentScope session)
	/*[ELSE] JAVA_SPEC_VERSION >= 20 */
	/**
	 * The method invoked via Clinker generates a native thunk to create
	 * a native symbol that holds an entry point to the native function
	 * intended for the requested java method in upcall.
	 *
	 * @param target the upcall method handle to the requested java method
	 * @param methodType the MethodType of the upcall method handle
	 * @param descriptor the FunctionDescriptor of the upcall method handle
	 * @param session the MemorySession of the upcall method handle
	 * @return the native symbol
	 */
	public static MemorySegment make(MethodHandle target, MethodType methodType, FunctionDescriptor descriptor, MemorySession session)
	/*[ENDIF] JAVA_SPEC_VERSION >= 20 */
	{
		UpcallLinker upcallLinker = new UpcallLinker(target, methodType, descriptor, session);
		return UpcallStubs.makeUpcall(upcallLinker.entryPoint(), session);
	}

	/*[IF JAVA_SPEC_VERSION >= 21]*/
	/**
	 * A stub method intended in OpenJDK to support compilation.
	 *
	 * Note:
	 * SharedUtils.arrangeUpcallHelper() (totally featured with the
	 * signatures intended for OpenJDK) calls this method which
	 * is specific to the upcall specific code implemented in
	 * OpenJDK. To work around this case during the compilation,
	 * this stub method serves as a placeholder to minimize the
	 * changes in OpenJDK.
	 *
	 * @param methodType the MethodType of the upcall method handle
	 * @param abi the descriptor of the Application Binary Interface (ABI) on a given platform
	 * @param callingSequence the calling sequence converted by CallArranger from a C FunctionDescriptor
	 * @return a factory instance that wraps up the upcall specific code
	 * @throws UnsupportedOperationException in the case of the OpenJDK implementation for upcall
	 */
	public static UpcallStubFactory makeFactory(MethodType methodType, ABIDescriptor abi, CallingSequence callingSequence) {
		throw new UnsupportedOperationException();
	}

	/**
	 * The method constructs a factory instance (introduced in JDK21 to support the
	 * sharing of function descriptors) which wraps up the existing upcall specific
	 * code implemented in OpenJ9 to enable the callback in OpenJDK.
	 *
	 * @param methodType the MethodType of the upcall method handle
	 * @param descriptor the FunctionDescriptor of the upcall method handle
	 * @return a factory instance that wraps up the upcall specific code
	 */
	public static UpcallStubFactory makeFactory(MethodType methodType, FunctionDescriptor descriptor) {
		return (target, session) -> {
			return UpcallLinker.make(target, methodType, descriptor, session);
		};
	}
	/*[ENDIF] JAVA_SPEC_VERSION >= 21 */
}
