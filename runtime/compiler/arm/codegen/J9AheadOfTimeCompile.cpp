/*******************************************************************************
 * Copyright (c) 2000, 2020 IBM Corp. and others
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
 * [2] http://openjdk.java.net/legal/assembly-exception.html
 *
 * SPDX-License-Identifier: EPL-2.0 OR Apache-2.0 OR GPL-2.0 WITH Classpath-exception-2.0 OR LicenseRef-GPL-2.0 WITH Assembly-exception
 *******************************************************************************/

#include "codegen/AheadOfTimeCompile.hpp"
#include "codegen/ARMAOTRelocation.hpp"
#include "compile/SymbolReferenceTable.hpp"
#include "codegen/CodeGenerator.hpp"
#include "env/FrontEnd.hpp"
#include "codegen/Instruction.hpp"
#include "compile/AOTClassInfo.hpp"
#include "compile/Compilation.hpp"
#include "compile/ResolvedMethod.hpp"
#include "compile/VirtualGuard.hpp"
#include "env/CHTable.hpp"
#include "env/ClassLoaderTable.hpp"
#include "env/SharedCache.hpp"
#include "env/jittypes.h"
#include "env/VMJ9.h"
#include "il/LabelSymbol.hpp"
#include "il/Node.hpp"
#include "il/Node_inlines.hpp"
#include "il/StaticSymbol.hpp"
#include "il/SymbolReference.hpp"
#include "runtime/RelocationRuntime.hpp"
#include "runtime/RelocationRecord.hpp"

#define  NON_HELPER         0

J9::ARM::AheadOfTimeCompile::AheadOfTimeCompile(TR::CodeGenerator *cg)
   : J9::AheadOfTimeCompile(_relocationTargetTypeToHeaderSizeMap, cg->comp()),
     _cg(cg),
     _relocationList(self()->trMemory())
   {
   }

void J9::ARM::AheadOfTimeCompile::processRelocations()
   {
   TR_J9VMBase *fej9 = (TR_J9VMBase *)(self()->cg()->fe());
   ListIterator<TR::ARMRelocation>  iterator(&self()->getRelocationList());
   TR::ARMRelocation               *relocation;
   TR::IteratedExternalRelocation  *r;

   for (relocation=iterator.getFirst();
        relocation!=NULL;
        relocation=iterator.getNext())
      {
      relocation->mapRelocation(self()->cg());
      }

   for (auto aotIterator = self()->cg()->getExternalRelocationList().begin(); aotIterator != self()->cg()->getExternalRelocationList().end(); ++aotIterator)
      {
	  (*aotIterator)->addExternalRelocation(self()->cg());
      }

   for (r = self()->getAOTRelocationTargets().getFirst();
        r != NULL;
        r = r->getNext())
      {
      self()->addToSizeOfAOTRelocations(r->getSizeOfRelocationData());
      }

   // now allocate the memory  size of all iterated relocations + the header (total length field)

   if (self()->getSizeOfAOTRelocations() != 0)
      {
      uint8_t *relocationDataCursor = self()->setRelocationData(fej9->allocateRelocationData(self()->comp(), self()->getSizeOfAOTRelocations() + 4));

      // set up the size for the region
      *(uint32_t *)relocationDataCursor = self()->getSizeOfAOTRelocations() + 4;
      relocationDataCursor += 4;

      // set up pointers for each iterated relocation and initialize header
      TR::IteratedExternalRelocation *s;
      for (s = self()->getAOTRelocationTargets().getFirst();
           s != NULL;
           s = s->getNext())
         {
         s->setRelocationData(relocationDataCursor);
         s->initializeRelocation(_cg);
         relocationDataCursor += s->getSizeOfRelocationData();
         }
      }
   }

uint8_t *J9::ARM::AheadOfTimeCompile::initializeAOTRelocationHeader(TR::IteratedExternalRelocation *relocation)
   {
   TR::Compilation* comp = TR::comp();
   TR_J9VMBase *fej9 = (TR_J9VMBase *)(_cg->fe());
   TR_SharedCache *sharedCache = fej9->sharedCache();
   TR_RelocationRuntime *reloRuntime = comp->reloRuntime();
   TR_RelocationTarget *reloTarget = reloRuntime->reloTarget();

   uint8_t * aotMethodCodeStart = (uint8_t *) comp->getRelocatableMethodCodeStart();
   uint8_t flags = 0;

   uint8_t *cursor         = relocation->getRelocationData();
   uint8_t targetKind      = relocation->getTargetKind();
   uint16_t sizeOfReloData = relocation->getSizeOfRelocationData();

   // Zero-initialize header
   memset(cursor, 0, sizeOfReloData);

   TR_RelocationRecord storage;
   TR_RelocationRecord *reloRecord = TR_RelocationRecord::create(&storage, reloRuntime, targetKind, reinterpret_cast<TR_RelocationRecordBinaryTemplate *>(cursor));

   uint8_t wideOffsets = relocation->needsWideOffsets() ? RELOCATION_TYPE_WIDE_OFFSET : 0;
   reloRecord->setSize(reloTarget, sizeOfReloData);
   reloRecord->setType(reloTarget, static_cast<TR_RelocationRecordType>(targetKind));
   reloRecord->setFlag(reloTarget, wideOffsets);

   switch (targetKind)
      {
      case TR_MethodObject:
         {
         TR_RelocationRecordMethodObject *moRecord = reinterpret_cast<TR_RelocationRecordMethodObject *>(reloRecord);
         TR_RelocationRecordInformation *recordInfo = (TR_RelocationRecordInformation*) relocation->getTargetAddress();

         TR::SymbolReference *symRef = reinterpret_cast<TR::SymbolReference *>(recordInfo->data1);
         uintptr_t inlinedSiteIndex = self()->findCorrectInlinedSiteIndex(symRef->getOwningMethod(comp)->constantPool(), recordInfo->data2);
         uint8_t flags = static_cast<uint8_t>(reinterpret_cast<uintptr_t>(recordInfo->data3));

         TR_ASSERT((flags & RELOCATION_CROSS_PLATFORM_FLAGS_MASK) == 0,  "reloFlags bits overlap cross-platform flags bits\n");

         moRecord->setInlinedSiteIndex(reloTarget, reinterpret_cast<uintptr_t>(inlinedSiteIndex));
         moRecord->setConstantPool(reloTarget, reinterpret_cast<uintptr_t>(symRef->getOwningMethod(comp)->constantPool()));
         moRecord->setReloFlags(reloTarget, flags);

         cursor = relocation->getRelocationData() + TR_RelocationRecord::getSizeOfAOTRelocationHeader(static_cast<TR_RelocationRecordType>(targetKind));
         }
         break;

      case TR_ClassAddress:
         {
         TR_RelocationRecordClassAddress *caRecord = reinterpret_cast<TR_RelocationRecordClassAddress *>(reloRecord);
         TR_RelocationRecordInformation *recordInfo = (TR_RelocationRecordInformation*) relocation->getTargetAddress();

         TR::SymbolReference *symRef = reinterpret_cast<TR::SymbolReference *>(recordInfo->data1);
         uintptr_t inlinedSiteIndex = reinterpret_cast<uintptr_t>(recordInfo->data2);
         uint8_t flags = static_cast<uint8_t>(recordInfo->data3);

         void *constantPool = symRef->getOwningMethod(comp)->constantPool();
         inlinedSiteIndex = self()->findCorrectInlinedSiteIndex(constantPool, inlinedSiteIndex);

         TR_ASSERT((flags & RELOCATION_CROSS_PLATFORM_FLAGS_MASK) == 0,  "reloFlags bits overlap cross-platform flags bits\n");
         caRecord->setReloFlags(reloTarget, flags);
         caRecord->setInlinedSiteIndex(reloTarget, inlinedSiteIndex);
         caRecord->setConstantPool(reloTarget, reinterpret_cast<uintptr_t>(constantPool));
         caRecord->setCpIndex(reloTarget, symRef->getCPIndex());

         cursor = relocation->getRelocationData() + TR_RelocationRecord::getSizeOfAOTRelocationHeader(static_cast<TR_RelocationRecordType>(targetKind));
         }
         break;

      case TR_DataAddress:
         {
         TR_RelocationRecordDataAddress *daRecord = reinterpret_cast<TR_RelocationRecordDataAddress *>(reloRecord);
         TR_RelocationRecordInformation *recordInfo = (TR_RelocationRecordInformation*) relocation->getTargetAddress();

         TR::SymbolReference *symRef = reinterpret_cast<TR::SymbolReference *>(recordInfo->data1);
         uintptr_t inlinedSiteIndex = reinterpret_cast<uintptr_t>(recordInfo->data2);
         uint8_t flags = static_cast<uint8_t>(reinterpret_cast<uintptr_t>(recordInfo->data3));

         void *constantPool = symRef->getOwningMethod(comp)->constantPool();
         inlinedSiteIndex = self()->findCorrectInlinedSiteIndex(constantPool, inlinedSiteIndex);

         TR_ASSERT((flags & RELOCATION_CROSS_PLATFORM_FLAGS_MASK) == 0,  "reloFlags bits overlap cross-platform flags bits\n");
         daRecord->setReloFlags(reloTarget, flags);
         daRecord->setInlinedSiteIndex(reloTarget, inlinedSiteIndex);
         daRecord->setConstantPool(reloTarget, reinterpret_cast<uintptr_t>(constantPool));
         daRecord->setCpIndex(reloTarget, symRef->getCPIndex());
         daRecord->setOffset(reloTarget, symRef->getOffset());

         cursor = relocation->getRelocationData() + TR_RelocationRecord::getSizeOfAOTRelocationHeader(static_cast<TR_RelocationRecordType>(targetKind));
         }
         break;

      case TR_FixedSequenceAddress2:
         {
         TR_RelocationRecordWithOffset *rwoRecord = reinterpret_cast<TR_RelocationRecordWithOffset *>(reloRecord);
         uint8_t flags = static_cast<uint8_t>(reinterpret_cast<uintptr_t>(relocation->getTargetAddress2()));

         TR_ASSERT((flags & RELOCATION_CROSS_PLATFORM_FLAGS_MASK) == 0,  "reloFlags bits overlap cross-platform flags bits\n");
         rwoRecord->setReloFlags(reloTarget, flags);

         uintptr_t offset = relocation->getTargetAddress()
                  ? static_cast<uintptr_t>(relocation->getTargetAddress() - aotMethodCodeStart)
                  : 0x0;

         rwoRecord->setOffset(reloTarget, offset);

         cursor = relocation->getRelocationData() + TR_RelocationRecord::getSizeOfAOTRelocationHeader(static_cast<TR_RelocationRecordType>(targetKind));
         }
         break;

      case TR_BodyInfoAddressLoad:
         {
         TR_RelocationRecord *rRecord = reinterpret_cast<TR_RelocationRecord *>(reloRecord);

         uint8_t flags = flags = static_cast<uint8_t>(reinterpret_cast<uintptr_t>(relocation->getTargetAddress2()));
         TR_ASSERT((flags & RELOCATION_CROSS_PLATFORM_FLAGS_MASK) == 0,  "reloFlags bits overlap cross-platform flags bits\n");
         rRecord->setReloFlags(reloTarget, flags);

         cursor = relocation->getRelocationData() + TR_RelocationRecord::getSizeOfAOTRelocationHeader(static_cast<TR_RelocationRecordType>(targetKind));
         }
         break;

      case TR_RamMethodSequence:
         {
         TR_RelocationRecordRamSequence *rsRecord = reinterpret_cast<TR_RelocationRecordRamSequence *>(reloRecord);
         uint8_t flags = static_cast<uint8_t>(reinterpret_cast<uintptr_t>(relocation->getTargetAddress2()));

         TR_ASSERT((flags & RELOCATION_CROSS_PLATFORM_FLAGS_MASK) == 0,  "reloFlags bits overlap cross-platform flags bits\n");
         rsRecord->setReloFlags(reloTarget, flags);

         // Skip Offset

         cursor = relocation->getRelocationData() + TR_RelocationRecord::getSizeOfAOTRelocationHeader(static_cast<TR_RelocationRecordType>(targetKind));
         }
         break;

      case TR_GlobalValue:
      case TR_HCR:
         {
         TR_RelocationRecordWithOffset *rwoRecord = reinterpret_cast<TR_RelocationRecordWithOffset *>(reloRecord);

         uintptr_t gv = reinterpret_cast<uintptr_t>(relocation->getTargetAddress());
         uint8_t flags = static_cast<uint8_t>(reinterpret_cast<uintptr_t>(relocation->getTargetAddress2()));

         TR_ASSERT((flags & RELOCATION_CROSS_PLATFORM_FLAGS_MASK) == 0,  "reloFlags bits overlap cross-platform flags bits\n");
         rwoRecord->setReloFlags(reloTarget, flags);
         rwoRecord->setOffset(reloTarget, gv);

         cursor = relocation->getRelocationData() + TR_RelocationRecord::getSizeOfAOTRelocationHeader(static_cast<TR_RelocationRecordType>(targetKind));
         }
         break;

      case TR_ArbitraryClassAddress:
         {
         TR_RelocationRecordArbitraryClassAddress *acaRecord = reinterpret_cast<TR_RelocationRecordArbitraryClassAddress *>(reloRecord);

         // ExternalRelocation data is as expected for TR_ClassAddress
         TR_RelocationRecordInformation *recordInfo = (TR_RelocationRecordInformation*) relocation->getTargetAddress();

         auto symRef = (TR::SymbolReference *)recordInfo->data1;
         auto sym = symRef->getSymbol()->castToStaticSymbol();
         auto j9class = (TR_OpaqueClassBlock *)sym->getStaticAddress();
         // flags stored in data3 are currently unused
         uintptr_t inlinedSiteIndex = self()->findCorrectInlinedSiteIndex(symRef->getOwningMethod(comp)->constantPool(), recordInfo->data2);

         uintptr_t classChainIdentifyingLoaderOffsetInSharedCache = sharedCache->getClassChainOffsetOfIdentifyingLoaderForClazzInSharedCache(j9class);
         uintptr_t classChainOffsetInSharedCache = self()->getClassChainOffset(j9class);

         acaRecord->setInlinedSiteIndex(reloTarget, inlinedSiteIndex);
         acaRecord->setClassChainIdentifyingLoaderOffsetInSharedCache(reloTarget, classChainIdentifyingLoaderOffsetInSharedCache);
         acaRecord->setClassChainForInlinedMethod(reloTarget, classChainOffsetInSharedCache);

         cursor = relocation->getRelocationData() + TR_RelocationRecord::getSizeOfAOTRelocationHeader(static_cast<TR_RelocationRecordType>(targetKind));
         }
         break;

      default:
         cursor = self()->initializeCommonAOTRelocationHeader(relocation, reloRecord);

      }
      return cursor;
   }


uint32_t J9::ARM::AheadOfTimeCompile::_relocationTargetTypeToHeaderSizeMap[TR_NumExternalRelocationKinds] =
   {
   12,                                       // TR_ConstantPool                        = 0
   8,                                        // TR_HelperAddress                       = 1
   12,                                       // TR_RelativeMethodAddress               = 2
   4,                                        // TR_AbsoluteMethodAddress               = 3
   20,                                       // TR_DataAddress                         = 4
   12,                                       // TR_ClassObject                         = 5
   12,                                       // TR_MethodObject                        = 6
   12,                                       // TR_InterfaceObject                     = 7
   8,                                        // TR_AbsoluteHelperAddress               = 8
   8,                                        // TR_FixedSequenceAddress                = 9
   8,                                        // TR_FixedSequenceAddress2               = 10
   16,                                       // TR_JNIVirtualTargetAddress             = 11
   16,                                       // TR_JNIStaticTargetAddress              = 12
   4,                                        // TR_ArrayCopyHelper                     = 13
   4,                                        // TR_ArrayCopyToc                        = 14
   4,                                        // TR_BodyInfoAddress                     = 15
   12,                                       // TR_Thunks                              = 16
   16,                                       // TR_StaticRamMethodConst                = 17
   12,                                       // TR_Trampolines                         = 18
   8,                                        // TR_PicTrampolines                      = 19
   8,                                        // TR_CheckMethodEnter                    = 20
   4,                                        // TR_RamMethod                           = 21
   8,                                        // TR_RamMethodSequence                   = 22
   8,                                        // TR_RamMethodSequenceReg                = 23
   24,                                       // TR_VerifyClassObjectForAlloc           = 24
   12,                                       // TR_ConstantPoolOrderedPair             = 25
   4,                                        // TR_AbsoluteMethodAddressOrderedPair    = 26
   20,                                       // TR_VerifyRefArrayForAlloc              = 27
   12,                                       // TR_J2IThunks                           = 28
   8,                                        // TR_GlobalValue                         = 29
   4,                                        // TR_BodyInfoAddressLoad                 = 30
   20,                                       // TR_ValidateInstanceField               = 31
   24,                                       // TR_InlinedStaticMethodWithNopGuard     = 32
   24,                                       // TR_InlinedSpecialMethodWithNopGuard    = 33
   24,                                       // TR_InlinedVirtualMethodWithNopGuard    = 34
   24,                                       // TR_InlinedInterfaceMethodWithNopGuard  = 35
   16,                                       // TR_SpecialRamMethodConst               = 36
   24,                                       // TR_InlinedHCRMethod                    = 37
   20,                                       // TR_ValidateStaticField                 = 38
   20,                                       // TR_ValidateClass                       = 39
   16,                                       // TR_ClassAddress                        = 40
   8,                                        // TR_HCR                                 = 41
   32,                                       // TR_ProfiledMethodGuardRelocation       = 42
   32,                                       // TR_ProfiledClassGuardRelocation        = 43
   0,                                        // TR_HierarchyGuardRelocation            = 44
   0,                                        // TR_AbstractGuardRelocation             = 45
   32,                                       // TR_ProfiledInlinedMethod               = 46
   20,                                       // TR_MethodPointer                       = 47
   16,                                       // TR_ClassPointer                        = 48
   8,                                        // TR_CheckMethodExit                     = 49
   12,                                       // TR_ValidateArbitraryClass              = 50
   0,                                        // TR_EmitClass(not used)                 = 51
   16,                                       // TR_JNISpecialTargetAddress             = 52
   16,                                       // TR_VirtualRamMethodConst               = 53
   20,                                       // TR_InlinedInterfaceMethod              = 54
   20,                                       // TR_InlinedVirtualMethod                = 55
   0,                                        // TR_NativeMethodAbsolute                = 56,
   0,                                        // TR_NativeMethodRelative                = 57,
   16,                                       // TR_ArbitraryClassAddress               = 58,
   28,                                        // TR_DebugCounter                        = 59
   4,                                        // TR_ClassUnloadAssumption               = 60
   16,                                       // TR_J2IVirtualThunkPointer              = 61
   };


#if 0

void J9::ARM::AheadOfTimeCompile::dumpRelocationData()
   {
   TR::Compilation *comp = TR::comp();
   uint8_t *cursor = getRelocationData();
   if (cursor)
      {
      uint8_t *endOfData = cursor + *(uint32_t *)cursor;
      diagnostic("Size of relocation data in AOT object is %d bytes\n"
                  "Size field in relocation data is         %d bytes\n", getSizeOfAOTRelocations(), *(uint32_t *)cursor);
      cursor += 4;

      while (cursor < endOfData)
         {
         diagnostic("\nSize of relocation is %d bytes.\n", *(uint16_t *)cursor);
         uint8_t *endOfCurrentRecord = cursor + *(uint16_t *)cursor;
         cursor += 2;
         TR_ExternalRelocationTargetKind kind = (TR_ExternalRelocationTargetKind)(*cursor & TR_ExternalRelocationTargetKindMask);
         diagnostic("Relocation type is %s.\n", TR::ExternalRelocation::getName(kind));
         int32_t offsetSize = (*cursor & RELOCATION_TYPE_WIDE_OFFSET) ? 4 : 2;
         bool isOrderedPair = (*cursor & RELOCATION_TYPE_ORDERED_PAIR)? true : false;
         diagnostic("Relocation uses %d-byte wide offsets.\n", offsetSize);
         diagnostic("Relocation uses %s offsets.\n", isOrderedPair ? "ordered-pair" : "non-paired");
         diagnostic("Relocation is %s.\n", (*cursor & RELOCATION_TYPE_EIP_OFFSET) ? "EIP Relative" : "Absolute");
         cursor++;
         diagnostic( "Target Info:\n");
         switch (kind)
            {
            case TR_ConstantPool:
               // constant pool address is placed as the last word of the header
               diagnostic("Constant pool %x", *(uint32_t *)++cursor);
               cursor += 4;
               break;
            case TR_HelperAddress:
               {
               // final byte of header is the index which indicates the particular helper being relocated to
               TR::SymbolReference *tempSR = comp->getSymRefTab()->getSymRef((int32_t)*cursor);
               diagnostic("Helper method address of %s(%d)", comp->getDebug()->getName(tempSR), (int32_t)*cursor++);
               }
               break;
            case TR_RelativeMethodAddress:
               // next word is the address of the constant pool to which the index refers
               // second word is the index in the above stored constant pool that indicates the particular
               // relocation target
               diagnostic("Relative Method Address: Constant pool = %x, index = %d", *(uint32_t *)(cursor+1), *(uint32_t *)(cursor+5));
               cursor += 9;
               break;
            case TR_AbsoluteMethodAddress:
               // Reference to the current method, no other information
               diagnostic("Absolute Method Address:");
               cursor++;
               break;
            case TR_DataAddress:
               // next word is the address of the constant pool to which the index refers
               // second word is the index in the above stored constant pool that indicates the particular
               // relocation target
               diagnostic("Data Address: Constant pool = %x, index = %d", *(uint32_t *)(cursor+1), *(uint32_t *)(cursor+5));
               cursor += 9;
               break;
            case TR_ClassObject:
               // next word is the address of the constant pool to which the index refers
               // second word is the index in the above stored constant pool that indicates the particular
               // relocation target
               diagnostic("Class Object: Constant pool = %x, index = %d", *(uint32_t *)(cursor+1), *(uint32_t *)(cursor+5));
               cursor += 9;
               break;
            case TR_MethodObject:
               // next word is the address of the constant pool to which the index refers
               // second word is the index in the above stored constant pool that indicates the particular
               // relocation target
               diagnostic("Method Object: Constant pool = %x, index = %d", *(uint32_t *)(cursor+1), *(uint32_t *)(cursor+5));
               cursor += 9;
               break;
            case TR_InterfaceObject:
               // next word is the address of the constant pool to which the index refers
               // second word is the index in the above stored constant pool that indicates the particular
               // relocation target
               diagnostic("Interface Object: Constant pool = %x, index = %d", *(uint32_t *)(cursor+1), *(uint32_t *)(cursor+5));
               cursor += 9;
               break;
            }
         diagnostic("\nUpdate location offsets:");
         uint8_t count = 0;
         if (offsetSize == 2)
            {
            while (cursor < endOfCurrentRecord)
               {
               if ((isOrderedPair && (count % 4)==0) ||
                   (!isOrderedPair && (count % 16)==0))
                  {
                  diagnostic("\n");
                  }
               count++;
               if (isOrderedPair)
		  {
                  diagnostic("(%04x ", *(uint16_t *)cursor);
                  cursor += 2;
                  diagnostic("%04x) ", *(uint16_t *)cursor);
                  cursor += 2;
		  }
               else
		  {
                  diagnostic("%04x ", *(uint16_t *)cursor);
                  cursor += 2;
		  }
               }
            }
         else
            {
            while (cursor < endOfCurrentRecord)
               {
               if ((isOrderedPair && (count % 2)==0) ||
                   (!isOrderedPair && (count % 8)==0))
                  {
                  diagnostic("\n");
                  }
               count++;
               if (isOrderedPair)
		  {
                  diagnostic("(%08x ", *(uint32_t *)cursor);
                  cursor += 4;
                  diagnostic("%08x) ", *(uint32_t *)cursor);
                  cursor += 4;
		  }
               else
		  {
                  diagnostic("%08x ", *(uint32_t *)cursor);
                  cursor += 4;
		  }
               }
            }
         diagnostic("\n");
         }

      diagnostic("\n\n");
      }
   else
      {
      diagnostic("No relocation data allocated\n");
      }
   }


#endif
