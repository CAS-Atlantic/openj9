/*[INCLUDE-IF !OPENJDK_METHODHANDLES & !VENDOR_UMA]*/
/*
 * Copyright IBM Corp. and others 2017
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
 */
package java.lang.invoke;

/*[IF JAVA_SPEC_VERSION >= 15]*/
import java.util.List;
/*[ENDIF] JAVA_SPEC_VERSION >= 15 */

/*
 * Stub class to compile OpenJDK j.l.i.MethodHandleImpl
 */
final class SimpleMethodHandle extends BoundMethodHandle {

	private SimpleMethodHandle(MethodType type, LambdaForm form) {
		super(type, form);
		OpenJDKCompileStub.OpenJDKCompileStubThrowError();
	}

	BoundMethodHandle copyWith(MethodType mt, LambdaForm lf) {
		throw OpenJDKCompileStub.OpenJDKCompileStubThrowError();
	}

	static BoundMethodHandle make(MethodType mt, LambdaForm lf) {
		throw OpenJDKCompileStub.OpenJDKCompileStubThrowError();
	}

	BoundMethodHandle copyWithExtendL(MethodType mt, LambdaForm lf, Object obj) {
		throw OpenJDKCompileStub.OpenJDKCompileStubThrowError();
	}

/*[IF JAVA_SPEC_VERSION >= 15]*/
	@Override
	boolean addRelatedMHs(List<MethodHandle> relatedMHs) {
		return false;
	}
/*[ENDIF] JAVA_SPEC_VERSION >= 15 */
}
