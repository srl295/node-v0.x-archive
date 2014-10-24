// Copyright 2012 the V8 project authors. All rights reserved.
//
// Copyright IBM Corp. 2012, 2013. All rights reserved.
//
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "src/v8.h"

#if V8_TARGET_ARCH_PPC

#include "src/codegen.h"
#include "src/ic-inl.h"
#include "src/stub-cache.h"

namespace v8 {
namespace internal {

#define __ ACCESS_MASM(masm)


static void ProbeTable(Isolate* isolate,
                       MacroAssembler* masm,
                       Code::Flags flags,
                       StubCache::Table table,
                       Register receiver,
                       Register name,
                       // Number of the cache entry, not scaled.
                       Register offset,
                       Register scratch,
                       Register scratch2,
                       Register offset_scratch) {
  ExternalReference key_offset(isolate->stub_cache()->key_reference(table));
  ExternalReference value_offset(isolate->stub_cache()->value_reference(table));
  ExternalReference map_offset(isolate->stub_cache()->map_reference(table));

  uintptr_t key_off_addr = reinterpret_cast<uintptr_t>(key_offset.address());
  uintptr_t value_off_addr =
                         reinterpret_cast<uintptr_t>(value_offset.address());
  uintptr_t map_off_addr = reinterpret_cast<uintptr_t>(map_offset.address());

  // Check the relative positions of the address fields.
  DCHECK(value_off_addr > key_off_addr);
  DCHECK((value_off_addr - key_off_addr) % 4 == 0);
  DCHECK((value_off_addr - key_off_addr) < (256 * 4));
  DCHECK(map_off_addr > key_off_addr);
  DCHECK((map_off_addr - key_off_addr) % 4 == 0);
  DCHECK((map_off_addr - key_off_addr) < (256 * 4));

  Label miss;
  Register base_addr = scratch;
  scratch = no_reg;

  // Multiply by 3 because there are 3 fields per entry (name, code, map).
  __ ShiftLeftImm(offset_scratch, offset, Operand(1));
  __ add(offset_scratch, offset, offset_scratch);

  // Calculate the base address of the entry.
  __ mov(base_addr, Operand(key_offset));
#if V8_TARGET_ARCH_PPC64
  DCHECK(kPointerSizeLog2 > StubCache::kCacheIndexShift);
  __ ShiftLeftImm(offset_scratch, offset_scratch,
                  Operand(kPointerSizeLog2 - StubCache::kCacheIndexShift));
#else
  DCHECK(kPointerSizeLog2 == StubCache::kCacheIndexShift);
#endif
  __ add(base_addr, base_addr, offset_scratch);

  // Check that the key in the entry matches the name.
  __ LoadP(ip, MemOperand(base_addr, 0));
  __ cmp(name, ip);
  __ bne(&miss);

  // Check the map matches.
  __ LoadP(ip, MemOperand(base_addr, map_off_addr - key_off_addr));
  __ LoadP(scratch2, FieldMemOperand(receiver, HeapObject::kMapOffset));
  __ cmp(ip, scratch2);
  __ bne(&miss);

  // Get the code entry from the cache.
  Register code = scratch2;
  scratch2 = no_reg;
  __ LoadP(code, MemOperand(base_addr, value_off_addr - key_off_addr));

  // Check that the flags match what we're looking for.
  Register flags_reg = base_addr;
  base_addr = no_reg;
  __ lwz(flags_reg, FieldMemOperand(code, Code::kFlagsOffset));

  DCHECK(!r0.is(flags_reg));
  __ li(r0, Operand(Code::kFlagsNotUsedInLookup));
  __ andc(flags_reg, flags_reg, r0);
  __ mov(r0, Operand(flags));
  __ cmpl(flags_reg, r0);
  __ bne(&miss);

#ifdef DEBUG
    if (FLAG_test_secondary_stub_cache && table == StubCache::kPrimary) {
      __ b(&miss);
    } else if (FLAG_test_primary_stub_cache && table == StubCache::kSecondary) {
      __ b(&miss);
    }
#endif

  // Jump to the first instruction in the code stub.
  __ addi(r0, code, Operand(Code::kHeaderSize - kHeapObjectTag));
  __ mtctr(r0);
  __ bctr();

  // Miss: fall through.
  __ bind(&miss);
}


void PropertyHandlerCompiler::GenerateDictionaryNegativeLookup(
    MacroAssembler* masm, Label* miss_label, Register receiver,
    Handle<Name> name, Register scratch0, Register scratch1) {
  DCHECK(name->IsUniqueName());
  DCHECK(!receiver.is(scratch0));
  Counters* counters = masm->isolate()->counters();
  __ IncrementCounter(counters->negative_lookups(), 1, scratch0, scratch1);
  __ IncrementCounter(counters->negative_lookups_miss(), 1, scratch0, scratch1);

  Label done;

  const int kInterceptorOrAccessCheckNeededMask =
      (1 << Map::kHasNamedInterceptor) | (1 << Map::kIsAccessCheckNeeded);

  // Bail out if the receiver has a named interceptor or requires access checks.
  Register map = scratch1;
  __ LoadP(map, FieldMemOperand(receiver, HeapObject::kMapOffset));
  __ lbz(scratch0, FieldMemOperand(map, Map::kBitFieldOffset));
  __ andi(r0, scratch0, Operand(kInterceptorOrAccessCheckNeededMask));
  __ bne(miss_label, cr0);

  // Check that receiver is a JSObject.
  __ lbz(scratch0, FieldMemOperand(map, Map::kInstanceTypeOffset));
  __ cmpi(scratch0, Operand(FIRST_SPEC_OBJECT_TYPE));
  __ blt(miss_label);

  // Load properties array.
  Register properties = scratch0;
  __ LoadP(properties, FieldMemOperand(receiver, JSObject::kPropertiesOffset));
  // Check that the properties array is a dictionary.
  __ LoadP(map, FieldMemOperand(properties, HeapObject::kMapOffset));
  Register tmp = properties;
  __ LoadRoot(tmp, Heap::kHashTableMapRootIndex);
  __ cmp(map, tmp);
  __ bne(miss_label);

  // Restore the temporarily used register.
  __ LoadP(properties, FieldMemOperand(receiver, JSObject::kPropertiesOffset));


  NameDictionaryLookupStub::GenerateNegativeLookup(masm,
                                                   miss_label,
                                                   &done,
                                                   receiver,
                                                   properties,
                                                   name,
                                                   scratch1);
  __ bind(&done);
  __ DecrementCounter(counters->negative_lookups_miss(), 1, scratch0, scratch1);
}


void StubCache::GenerateProbe(MacroAssembler* masm,
                              Code::Flags flags,
                              Register receiver,
                              Register name,
                              Register scratch,
                              Register extra,
                              Register extra2,
                              Register extra3) {
  Isolate* isolate = masm->isolate();
  Label miss;

#if V8_TARGET_ARCH_PPC64
  // Make sure that code is valid. The multiplying code relies on the
  // entry size being 24.
  DCHECK(sizeof(Entry) == 24);
#else
  // Make sure that code is valid. The multiplying code relies on the
  // entry size being 12.
  DCHECK(sizeof(Entry) == 12);
#endif

  // Make sure the flags does not name a specific type.
  DCHECK(Code::ExtractTypeFromFlags(flags) == 0);

  // Make sure that there are no register conflicts.
  DCHECK(!scratch.is(receiver));
  DCHECK(!scratch.is(name));
  DCHECK(!extra.is(receiver));
  DCHECK(!extra.is(name));
  DCHECK(!extra.is(scratch));
  DCHECK(!extra2.is(receiver));
  DCHECK(!extra2.is(name));
  DCHECK(!extra2.is(scratch));
  DCHECK(!extra2.is(extra));

  // Check scratch, extra and extra2 registers are valid.
  DCHECK(!scratch.is(no_reg));
  DCHECK(!extra.is(no_reg));
  DCHECK(!extra2.is(no_reg));
  DCHECK(!extra3.is(no_reg));

  Counters* counters = masm->isolate()->counters();
  __ IncrementCounter(counters->megamorphic_stub_cache_probes(), 1,
                      extra2, extra3);

  // Check that the receiver isn't a smi.
  __ JumpIfSmi(receiver, &miss);

  // Get the map of the receiver and compute the hash.
  __ lwz(scratch, FieldMemOperand(name, Name::kHashFieldOffset));
  __ LoadP(ip, FieldMemOperand(receiver, HeapObject::kMapOffset));
  __ add(scratch, scratch, ip);
  __ xori(scratch, scratch, Operand(flags));
  // The mask omits the last two bits because they are not part of the hash.
  __ andi(scratch, scratch,
          Operand((kPrimaryTableSize - 1) << kCacheIndexShift));

  // Probe the primary table.
  ProbeTable(isolate,
             masm,
             flags,
             kPrimary,
             receiver,
             name,
             scratch,
             extra,
             extra2,
             extra3);

  // Primary miss: Compute hash for secondary probe.
  __ sub(scratch, scratch, name);
  __ addi(scratch, scratch, Operand(flags));
  __ andi(scratch, scratch,
          Operand((kSecondaryTableSize - 1) << kCacheIndexShift));

  // Probe the secondary table.
  ProbeTable(isolate,
             masm,
             flags,
             kSecondary,
             receiver,
             name,
             scratch,
             extra,
             extra2,
             extra3);

  // Cache miss: Fall-through and let caller handle the miss by
  // entering the runtime system.
  __ bind(&miss);
  __ IncrementCounter(counters->megamorphic_stub_cache_misses(), 1,
                      extra2, extra3);
}


void NamedLoadHandlerCompiler::GenerateDirectLoadGlobalFunctionPrototype(
    MacroAssembler* masm, int index, Register prototype, Label* miss) {
  Isolate* isolate = masm->isolate();
  // Get the global function with the given index.
  Handle<JSFunction> function(
      JSFunction::cast(isolate->native_context()->get(index)));

  // Check we're still in the same context.
  Register scratch = prototype;
  const int offset = Context::SlotOffset(Context::GLOBAL_OBJECT_INDEX);
  __ LoadP(scratch, MemOperand(cp, offset));
  __ LoadP(scratch, FieldMemOperand(scratch,
                                    GlobalObject::kNativeContextOffset));
  __ LoadP(scratch, MemOperand(scratch, Context::SlotOffset(index)));
  __ Move(ip, function);
  __ cmp(ip, scratch);
  __ bne(miss);

  // Load its initial map. The global functions all have initial maps.
  __ Move(prototype, Handle<Map>(function->initial_map()));
  // Load the prototype from the initial map.
  __ LoadP(prototype, FieldMemOperand(prototype, Map::kPrototypeOffset));
}


void NamedLoadHandlerCompiler::GenerateLoadFunctionPrototype(
    MacroAssembler* masm, Register receiver, Register scratch1,
    Register scratch2, Label* miss_label) {
  __ TryGetFunctionPrototype(receiver, scratch1, scratch2, miss_label);
  __ mr(r3, scratch1);
  __ Ret();
}


// Generate code to check that a global property cell is empty. Create
// the property cell at compilation time if no cell exists for the
// property.
void PropertyHandlerCompiler::GenerateCheckPropertyCell(
    MacroAssembler* masm, Handle<JSGlobalObject> global, Handle<Name> name,
    Register scratch, Label* miss) {
  Handle<Cell> cell = JSGlobalObject::EnsurePropertyCell(global, name);
  DCHECK(cell->value()->IsTheHole());
  __ mov(scratch, Operand(cell));
  __ LoadP(scratch, FieldMemOperand(scratch, Cell::kValueOffset));
  __ LoadRoot(ip, Heap::kTheHoleValueRootIndex);
  __ cmp(scratch, ip);
  __ bne(miss);
}


static void PushInterceptorArguments(MacroAssembler* masm, Register receiver,
                                     Register holder, Register name,
                                     Handle<JSObject> holder_obj) {
  STATIC_ASSERT(NamedLoadHandlerCompiler::kInterceptorArgsNameIndex == 0);
  STATIC_ASSERT(NamedLoadHandlerCompiler::kInterceptorArgsInfoIndex == 1);
  STATIC_ASSERT(NamedLoadHandlerCompiler::kInterceptorArgsThisIndex == 2);
  STATIC_ASSERT(NamedLoadHandlerCompiler::kInterceptorArgsHolderIndex == 3);
  STATIC_ASSERT(NamedLoadHandlerCompiler::kInterceptorArgsLength == 4);
  __ push(name);
  Handle<InterceptorInfo> interceptor(holder_obj->GetNamedInterceptor());
  DCHECK(!masm->isolate()->heap()->InNewSpace(*interceptor));
  Register scratch = name;
  __ mov(scratch, Operand(interceptor));
  __ push(scratch);
  __ push(receiver);
  __ push(holder);
}


static void CompileCallLoadPropertyWithInterceptor(
    MacroAssembler* masm, Register receiver, Register holder, Register name,
    Handle<JSObject> holder_obj, IC::UtilityId id) {
  PushInterceptorArguments(masm, receiver, holder, name, holder_obj);
  __ CallExternalReference(ExternalReference(IC_Utility(id), masm->isolate()),
                           NamedLoadHandlerCompiler::kInterceptorArgsLength);
}


// Generate call to api function.
void PropertyHandlerCompiler::GenerateFastApiCall(
    MacroAssembler* masm, const CallOptimization& optimization,
    Handle<Map> receiver_map, Register receiver, Register scratch_in,
    bool is_store, int argc, Register* values) {
  DCHECK(!receiver.is(scratch_in));
  __ push(receiver);
  // Write the arguments to stack frame.
  for (int i = 0; i < argc; i++) {
    Register arg = values[argc - 1 - i];
    DCHECK(!receiver.is(arg));
    DCHECK(!scratch_in.is(arg));
    __ push(arg);
  }
  DCHECK(optimization.is_simple_api_call());

  // Abi for CallApiFunctionStub.
  Register callee = r3;
  Register call_data = r7;
  Register holder = r5;
  Register api_function_address = r4;

  // Put holder in place.
  CallOptimization::HolderLookup holder_lookup;
  Handle<JSObject> api_holder =
      optimization.LookupHolderOfExpectedType(receiver_map, &holder_lookup);
  switch (holder_lookup) {
    case CallOptimization::kHolderIsReceiver:
      __ Move(holder, receiver);
      break;
    case CallOptimization::kHolderFound:
      __ Move(holder, api_holder);
      break;
    case CallOptimization::kHolderNotFound:
      UNREACHABLE();
      break;
  }

  Isolate* isolate = masm->isolate();
  Handle<JSFunction> function = optimization.constant_function();
  Handle<CallHandlerInfo> api_call_info = optimization.api_call_info();
  Handle<Object> call_data_obj(api_call_info->data(), isolate);

  // Put callee in place.
  __ Move(callee, function);

  bool call_data_undefined = false;
  // Put call_data in place.
  if (isolate->heap()->InNewSpace(*call_data_obj)) {
    __ Move(call_data, api_call_info);
    __ LoadP(call_data,
         FieldMemOperand(call_data, CallHandlerInfo::kDataOffset));
  } else if (call_data_obj->IsUndefined()) {
    call_data_undefined = true;
    __ LoadRoot(call_data, Heap::kUndefinedValueRootIndex);
  } else {
    __ Move(call_data, call_data_obj);
  }

  // Put api_function_address in place.
  Address function_address = v8::ToCData<Address>(api_call_info->callback());
  ApiFunction fun(function_address);
  ExternalReference::Type type = ExternalReference::DIRECT_API_CALL;
  ExternalReference ref = ExternalReference(&fun, type, masm->isolate());
  __ mov(api_function_address, Operand(ref));

  // Jump to stub.
  CallApiFunctionStub stub(isolate, is_store, call_data_undefined, argc);
  __ TailCallStub(&stub);
}


void PropertyAccessCompiler::GenerateTailCall(MacroAssembler* masm,
                                              Handle<Code> code) {
  __ Jump(code, RelocInfo::CODE_TARGET);
}


#undef __
#define __ ACCESS_MASM(masm())


void NamedStoreHandlerCompiler::GenerateRestoreName(Label* label,
                                                    Handle<Name> name) {
  if (!label->is_unused()) {
    __ bind(label);
    __ mov(this->name(), Operand(name));
  }
}


// Generate StoreTransition code, value is passed in r3 register.
// When leaving generated code after success, the receiver_reg and name_reg
// may be clobbered.  Upon branch to miss_label, the receiver and name
// registers have their original values.
void NamedStoreHandlerCompiler::GenerateStoreTransition(
    Handle<Map> transition, Handle<Name> name, Register receiver_reg,
    Register storage_reg, Register value_reg, Register scratch1,
    Register scratch2, Register scratch3, Label* miss_label, Label* slow) {
  // r3 : value
  Label exit;

  int descriptor = transition->LastAdded();
  DescriptorArray* descriptors = transition->instance_descriptors();
  PropertyDetails details = descriptors->GetDetails(descriptor);
  Representation representation = details.representation();
  DCHECK(!representation.IsNone());

  if (details.type() == CONSTANT) {
    Handle<Object> constant(descriptors->GetValue(descriptor), isolate());
    __ Move(scratch1, constant);
    __ cmp(value_reg, scratch1);
    __ bne(miss_label);
  } else if (representation.IsSmi()) {
    __ JumpIfNotSmi(value_reg, miss_label);
  } else if (representation.IsHeapObject()) {
    __ JumpIfSmi(value_reg, miss_label);
    HeapType* field_type = descriptors->GetFieldType(descriptor);
    HeapType::Iterator<Map> it = field_type->Classes();
    if (!it.Done()) {
      __ LoadP(scratch1, FieldMemOperand(value_reg, HeapObject::kMapOffset));
      Label do_store;
      while (true) {
        __ CompareMap(scratch1, it.Current(), &do_store);
        it.Advance();
        if (it.Done()) {
          __ bne(miss_label);
          break;
        }
        __ beq(&do_store);
      }
      __ bind(&do_store);
    }
  } else if (representation.IsDouble()) {
    Label do_store, heap_number;
    __ LoadRoot(scratch3, Heap::kMutableHeapNumberMapRootIndex);
    __ AllocateHeapNumber(storage_reg, scratch1, scratch2, scratch3, slow,
                          TAG_RESULT, MUTABLE);

    __ JumpIfNotSmi(value_reg, &heap_number);
    __ SmiUntag(scratch1, value_reg);
    __ ConvertIntToDouble(scratch1, d0);
    __ jmp(&do_store);

    __ bind(&heap_number);
    __ CheckMap(value_reg, scratch1, Heap::kHeapNumberMapRootIndex,
                miss_label, DONT_DO_SMI_CHECK);
    __ lfd(d0, FieldMemOperand(value_reg, HeapNumber::kValueOffset));

    __ bind(&do_store);
    __ stfd(d0, FieldMemOperand(storage_reg, HeapNumber::kValueOffset));
  }

  // Stub never generated for objects that require access checks.
  DCHECK(!transition->is_access_check_needed());

  // Perform map transition for the receiver if necessary.
  if (details.type() == FIELD &&
      Map::cast(transition->GetBackPointer())->unused_property_fields() == 0) {
    // The properties must be extended before we can store the value.
    // We jump to a runtime call that extends the properties array.
    __ push(receiver_reg);
    __ mov(r5, Operand(transition));
    __ Push(r5, r3);
    __ TailCallExternalReference(
        ExternalReference(IC_Utility(IC::kSharedStoreIC_ExtendStorage),
                          isolate()),
        3, 1);
    return;
  }

  // Update the map of the object.
  __ mov(scratch1, Operand(transition));
  __ StoreP(scratch1, FieldMemOperand(receiver_reg, HeapObject::kMapOffset),
            r0);

  // Update the write barrier for the map field.
  __ RecordWriteField(receiver_reg,
                      HeapObject::kMapOffset,
                      scratch1,
                      scratch2,
                      kLRHasNotBeenSaved,
                      kDontSaveFPRegs,
                      OMIT_REMEMBERED_SET,
                      OMIT_SMI_CHECK);

  if (details.type() == CONSTANT) {
    DCHECK(value_reg.is(r3));
    __ Ret();
    return;
  }

  int index = transition->instance_descriptors()->GetFieldIndex(
      transition->LastAdded());

  // Adjust for the number of properties stored in the object. Even in the
  // face of a transition we can use the old map here because the size of the
  // object and the number of in-object properties is not going to change.
  index -= transition->inobject_properties();

  // TODO(verwaest): Share this code as a code stub.
  SmiCheck smi_check = representation.IsTagged()
      ? INLINE_SMI_CHECK : OMIT_SMI_CHECK;
  if (index < 0) {
    // Set the property straight into the object.
    int offset = transition->instance_size() + (index * kPointerSize);
    if (representation.IsDouble()) {
      __ StoreP(storage_reg, FieldMemOperand(receiver_reg, offset), r0);
    } else {
      __ StoreP(value_reg, FieldMemOperand(receiver_reg, offset), r0);
    }

    if (!representation.IsSmi()) {
    // Update the write barrier for the array address.
      if (!representation.IsDouble()) {
        __ mr(storage_reg, value_reg);
      }
      __ RecordWriteField(receiver_reg,
                          offset,
                          storage_reg,
                          scratch1,
                          kLRHasNotBeenSaved,
                          kDontSaveFPRegs,
                          EMIT_REMEMBERED_SET,
                          smi_check);
    }
  } else {
    // Write to the properties array.
    int offset = index * kPointerSize + FixedArray::kHeaderSize;
    // Get the properties array
    __ LoadP(scratch1,
             FieldMemOperand(receiver_reg, JSObject::kPropertiesOffset));
    if (representation.IsDouble()) {
      __ StoreP(storage_reg, FieldMemOperand(scratch1, offset), r0);
    } else {
      __ StoreP(value_reg, FieldMemOperand(scratch1, offset), r0);
    }

    if (!representation.IsSmi()) {
      // Update the write barrier for the array address.
      if (!representation.IsDouble()) {
        __ mr(storage_reg, value_reg);
      }
      __ RecordWriteField(scratch1,
                          offset,
                          storage_reg,
                          receiver_reg,
                          kLRHasNotBeenSaved,
                          kDontSaveFPRegs,
                          EMIT_REMEMBERED_SET,
                          smi_check);
    }
  }

  // Return the value (register r3).
  DCHECK(value_reg.is(r3));
  __ bind(&exit);
  __ Ret();
}


void NamedStoreHandlerCompiler::GenerateStoreField(LookupResult* lookup,
                                                   Register value_reg,
                                                   Label* miss_label) {
  DCHECK(lookup->representation().IsHeapObject());
  __ JumpIfSmi(value_reg, miss_label);
  HeapType::Iterator<Map> it = lookup->GetFieldType()->Classes();
  __ LoadP(scratch1(), FieldMemOperand(value_reg, HeapObject::kMapOffset));
  Label do_store;
  while (true) {
    __ CompareMap(scratch1(), it.Current(), &do_store);
    it.Advance();
    if (it.Done()) {
      __ bne(miss_label);
      break;
    }
    __ beq(&do_store);
  }
  __ bind(&do_store);

  StoreFieldStub stub(isolate(), lookup->GetFieldIndex(),
                      lookup->representation());
  GenerateTailCall(masm(), stub.GetCode());
}


Register PropertyHandlerCompiler::CheckPrototypes(
    Register object_reg, Register holder_reg, Register scratch1,
    Register scratch2, Handle<Name> name, Label* miss,
    PrototypeCheckType check) {
  Handle<Map> receiver_map(IC::TypeToMap(*type(), isolate()));

  // Make sure there's no overlap between holder and object registers.
  DCHECK(!scratch1.is(object_reg) && !scratch1.is(holder_reg));
  DCHECK(!scratch2.is(object_reg) && !scratch2.is(holder_reg)
         && !scratch2.is(scratch1));

  // Keep track of the current object in register reg.
  Register reg = object_reg;
  int depth = 0;

  Handle<JSObject> current = Handle<JSObject>::null();
  if (type()->IsConstant()) {
    current = Handle<JSObject>::cast(type()->AsConstant()->Value());
  }
  Handle<JSObject> prototype = Handle<JSObject>::null();
  Handle<Map> current_map = receiver_map;
  Handle<Map> holder_map(holder()->map());
  // Traverse the prototype chain and check the maps in the prototype chain for
  // fast and global objects or do negative lookup for normal objects.
  while (!current_map.is_identical_to(holder_map)) {
    ++depth;

    // Only global objects and objects that do not require access
    // checks are allowed in stubs.
    DCHECK(current_map->IsJSGlobalProxyMap() ||
           !current_map->is_access_check_needed());

    prototype = handle(JSObject::cast(current_map->prototype()));
    if (current_map->is_dictionary_map() &&
        !current_map->IsJSGlobalObjectMap()) {
        DCHECK(!current_map->IsJSGlobalProxyMap());  // Proxy maps are fast.
      if (!name->IsUniqueName()) {
        DCHECK(name->IsString());
        name = factory()->InternalizeString(Handle<String>::cast(name));
      }
      DCHECK(current.is_null() ||
             current->property_dictionary()->FindEntry(name) ==
             NameDictionary::kNotFound);

      GenerateDictionaryNegativeLookup(masm(), miss, reg, name,
                                       scratch1, scratch2);

      __ LoadP(scratch1, FieldMemOperand(reg, HeapObject::kMapOffset));
      reg = holder_reg;  // From now on the object will be in holder_reg.
      __ LoadP(reg, FieldMemOperand(scratch1, Map::kPrototypeOffset));
    } else {
      Register map_reg = scratch1;
      if (depth != 1 || check == CHECK_ALL_MAPS) {
        // CheckMap implicitly loads the map of |reg| into |map_reg|.
        __ CheckMap(reg, map_reg, current_map, miss, DONT_DO_SMI_CHECK);
      } else {
        __ LoadP(map_reg, FieldMemOperand(reg, HeapObject::kMapOffset));
      }

      // Check access rights to the global object.  This has to happen after
      // the map check so that we know that the object is actually a global
      // object.
      // This allows us to install generated handlers for accesses to the
      // global proxy (as opposed to using slow ICs). See corresponding code
      // in LookupForRead().
      if (current_map->IsJSGlobalProxyMap()) {
        __ CheckAccessGlobalProxy(reg, scratch2, miss);
      } else if (current_map->IsJSGlobalObjectMap()) {
        GenerateCheckPropertyCell(
            masm(), Handle<JSGlobalObject>::cast(current), name,
            scratch2, miss);
      }

      reg = holder_reg;  // From now on the object will be in holder_reg.

      // Two possible reasons for loading the prototype from the map:
      // (1) Can't store references to new space in code.
      // (2) Handler is shared for all receivers with the same prototype
      //     map (but not necessarily the same prototype instance).
      bool load_prototype_from_map =
          heap()->InNewSpace(*prototype) || depth == 1;
      if (load_prototype_from_map) {
        __ LoadP(reg, FieldMemOperand(map_reg, Map::kPrototypeOffset));
      } else {
        __ mov(reg, Operand(prototype));
      }
    }

    // Go to the next object in the prototype chain.
    current = prototype;
    current_map = handle(current->map());
  }

  // Log the check depth.
  LOG(isolate(), IntEvent("check-maps-depth", depth + 1));

  if (depth != 0 || check == CHECK_ALL_MAPS) {
    // Check the holder map.
    __ CheckMap(reg, scratch1, current_map, miss, DONT_DO_SMI_CHECK);
  }

  // Perform security check for access to the global object.
  DCHECK(current_map->IsJSGlobalProxyMap() ||
         !current_map->is_access_check_needed());
  if (current_map->IsJSGlobalProxyMap()) {
    __ CheckAccessGlobalProxy(reg, scratch1, miss);
  }

  // Return the register containing the holder.
  return reg;
}


void NamedLoadHandlerCompiler::FrontendFooter(Handle<Name> name, Label* miss) {
  if (!miss->is_unused()) {
    Label success;
    __ b(&success);
    __ bind(miss);
    TailCallBuiltin(masm(), MissBuiltin(kind()));
    __ bind(&success);
  }
}


void NamedStoreHandlerCompiler::FrontendFooter(Handle<Name> name, Label* miss) {
  if (!miss->is_unused()) {
    Label success;
    __ b(&success);
    GenerateRestoreName(miss, name);
    TailCallBuiltin(masm(), MissBuiltin(kind()));
    __ bind(&success);
  }
}


void NamedLoadHandlerCompiler::GenerateLoadConstant(Handle<Object> value) {
  // Return the constant value.
  __ Move(r3, value);
  __ Ret();
}


void NamedLoadHandlerCompiler::GenerateLoadCallback(
    Register reg, Handle<ExecutableAccessorInfo> callback) {
  // Build AccessorInfo::args_ list on the stack and push property name below
  // the exit frame to make GC aware of them and store pointers to them.
  STATIC_ASSERT(PropertyCallbackArguments::kHolderIndex == 0);
  STATIC_ASSERT(PropertyCallbackArguments::kIsolateIndex == 1);
  STATIC_ASSERT(PropertyCallbackArguments::kReturnValueDefaultValueIndex == 2);
  STATIC_ASSERT(PropertyCallbackArguments::kReturnValueOffset == 3);
  STATIC_ASSERT(PropertyCallbackArguments::kDataIndex == 4);
  STATIC_ASSERT(PropertyCallbackArguments::kThisIndex == 5);
  STATIC_ASSERT(PropertyCallbackArguments::kArgsLength == 6);
  DCHECK(!scratch2().is(reg));
  DCHECK(!scratch3().is(reg));
  DCHECK(!scratch4().is(reg));
  __ push(receiver());
  if (heap()->InNewSpace(callback->data())) {
    __ Move(scratch3(), callback);
    __ LoadP(scratch3(), FieldMemOperand(scratch3(),
                                       ExecutableAccessorInfo::kDataOffset));
  } else {
    __ Move(scratch3(), Handle<Object>(callback->data(), isolate()));
  }
  __ push(scratch3());
  __ LoadRoot(scratch3(), Heap::kUndefinedValueRootIndex);
  __ mr(scratch4(), scratch3());
  __ Push(scratch3(), scratch4());
  __ mov(scratch4(),
         Operand(ExternalReference::isolate_address(isolate())));
  __ Push(scratch4(), reg);
  __ push(name());

  // Abi for CallApiGetter
  Register getter_address_reg = r5;

  Address getter_address = v8::ToCData<Address>(callback->getter());
  ApiFunction fun(getter_address);
  ExternalReference::Type type = ExternalReference::DIRECT_GETTER_CALL;
  ExternalReference ref = ExternalReference(&fun, type, isolate());
  __ mov(getter_address_reg, Operand(ref));

  CallApiGetterStub stub(isolate());
  __ TailCallStub(&stub);
}


void NamedLoadHandlerCompiler::GenerateLoadInterceptor(Register holder_reg,
                                                       LookupResult* lookup,
                                                       Handle<Name> name) {
  DCHECK(holder()->HasNamedInterceptor());
  DCHECK(!holder()->GetNamedInterceptor()->getter()->IsUndefined());

  // So far the most popular follow ups for interceptor loads are FIELD
  // and CALLBACKS, so inline only them, other cases may be added
  // later.
  bool compile_followup_inline = false;
  if (lookup->IsFound() && lookup->IsCacheable()) {
    if (lookup->IsField()) {
      compile_followup_inline = true;
    } else if (lookup->type() == CALLBACKS &&
               lookup->GetCallbackObject()->IsExecutableAccessorInfo()) {
      Handle<ExecutableAccessorInfo> callback(
          ExecutableAccessorInfo::cast(lookup->GetCallbackObject()));
      compile_followup_inline =
          callback->getter() != NULL &&
          ExecutableAccessorInfo::IsCompatibleReceiverType(isolate(), callback,
                                                           type());
    }
  }

  if (compile_followup_inline) {
    // Compile the interceptor call, followed by inline code to load the
    // property from further up the prototype chain if the call fails.
    // Check that the maps haven't changed.
    DCHECK(holder_reg.is(receiver()) || holder_reg.is(scratch1()));

    // Preserve the receiver register explicitly whenever it is different from
    // the holder and it is needed should the interceptor return without any
    // result. The CALLBACKS case needs the receiver to be passed into C++ code,
    // the FIELD case might cause a miss during the prototype check.
    bool must_perfrom_prototype_check = *holder() != lookup->holder();
    bool must_preserve_receiver_reg = !receiver().is(holder_reg) &&
        (lookup->type() == CALLBACKS || must_perfrom_prototype_check);

    // Save necessary data before invoking an interceptor.
    // Requires a frame to make GC aware of pushed pointers.
    {
      FrameAndConstantPoolScope frame_scope(masm(), StackFrame::INTERNAL);
      if (must_preserve_receiver_reg) {
        __ Push(receiver(), holder_reg, this->name());
      } else {
        __ Push(holder_reg, this->name());
      }
      // Invoke an interceptor.  Note: map checks from receiver to
      // interceptor's holder has been compiled before (see a caller
      // of this method.)
      CompileCallLoadPropertyWithInterceptor(
          masm(), receiver(), holder_reg, this->name(), holder(),
          IC::kLoadPropertyWithInterceptorOnly);

      // Check if interceptor provided a value for property.  If it's
      // the case, return immediately.
      Label interceptor_failed;
      __ LoadRoot(scratch1(), Heap::kNoInterceptorResultSentinelRootIndex);
      __ cmp(r3, scratch1());
      __ beq(&interceptor_failed);
      frame_scope.GenerateLeaveFrame();
      __ Ret();

      __ bind(&interceptor_failed);
      __ pop(this->name());
      __ pop(holder_reg);
      if (must_preserve_receiver_reg) {
        __ pop(receiver());
      }
      // Leave the internal frame.
    }

    GenerateLoadPostInterceptor(holder_reg, name, lookup);
  } else {  // !compile_followup_inline
    // Call the runtime system to load the interceptor.
    // Check that the maps haven't changed.
    PushInterceptorArguments(masm(), receiver(), holder_reg, this->name(),
                             holder());

    ExternalReference ref =
        ExternalReference(IC_Utility(IC::kLoadPropertyWithInterceptor),
                          isolate());
    __ TailCallExternalReference(
        ref, NamedLoadHandlerCompiler::kInterceptorArgsLength, 1);
  }
}


Handle<Code> NamedStoreHandlerCompiler::CompileStoreCallback(
    Handle<JSObject> object, Handle<Name> name,
    Handle<ExecutableAccessorInfo> callback) {
  Register holder_reg = Frontend(receiver(), name);

  __ Push(receiver(), holder_reg);  // receiver
  __ mov(ip, Operand(callback));  // callback info
  __ push(ip);
  __ mov(ip, Operand(name));
  __ Push(ip, value());

  // Do tail-call to the runtime system.
  ExternalReference store_callback_property =
      ExternalReference(IC_Utility(IC::kStoreCallbackProperty), isolate());
  __ TailCallExternalReference(store_callback_property, 5, 1);

  // Return the generated code.
  return GetCode(kind(), Code::FAST, name);
}


#undef __
#define __ ACCESS_MASM(masm)


void NamedStoreHandlerCompiler::GenerateStoreViaSetter(
    MacroAssembler* masm, Handle<HeapType> type, Register receiver,
    Handle<JSFunction> setter) {
  // ----------- S t a t e -------------
  //  -- lr    : return address
  // -----------------------------------
  {
    FrameAndConstantPoolScope scope(masm, StackFrame::INTERNAL);

    // Save value register, so we can restore it later.
    __ push(value());

    if (!setter.is_null()) {
      // Call the JavaScript setter with receiver and value on the stack.
      if (IC::TypeToMap(*type, masm->isolate())->IsJSGlobalObjectMap()) {
        // Swap in the global receiver.
        __ LoadP(receiver,
                 FieldMemOperand(receiver, JSGlobalObject::kGlobalProxyOffset));
      }
      __ Push(receiver, value());
      ParameterCount actual(1);
      ParameterCount expected(setter);
      __ InvokeFunction(setter, expected, actual,
                        CALL_FUNCTION, NullCallWrapper());
    } else {
      // If we generate a global code snippet for deoptimization only, remember
      // the place to continue after deoptimization.
      masm->isolate()->heap()->SetSetterStubDeoptPCOffset(masm->pc_offset());
    }

    // We have to return the passed value, not the return value of the setter.
    __ pop(r3);

    // Restore context register.
    __ LoadP(cp, MemOperand(fp, StandardFrameConstants::kContextOffset));
  }
  __ Ret();
}


#undef __
#define __ ACCESS_MASM(masm())


Handle<Code> NamedStoreHandlerCompiler::CompileStoreInterceptor(
    Handle<Name> name) {
  __ Push(receiver(), this->name(), value());

  // Do tail-call to the runtime system.
  ExternalReference store_ic_property = ExternalReference(
      IC_Utility(IC::kStorePropertyWithInterceptor), isolate());
  __ TailCallExternalReference(store_ic_property, 3, 1);

  // Return the generated code.
  return GetCode(kind(), Code::FAST, name);
}


Register* PropertyAccessCompiler::load_calling_convention() {
  // receiver, name, scratch1, scratch2, scratch3, scratch4.
  Register receiver = LoadIC::ReceiverRegister();
  Register name = LoadIC::NameRegister();
  static Register registers[] = { receiver, name, r6, r3, r7, r8 };
  return registers;
}


Register* PropertyAccessCompiler::store_calling_convention() {
  // receiver, name, scratch1, scratch2, scratch3.
  Register receiver = StoreIC::ReceiverRegister();
  Register name = StoreIC::NameRegister();
  DCHECK(r6.is(KeyedStoreIC::MapRegister()));
  static Register registers[] = { receiver, name, r6, r7, r8 };
  return registers;
}


Register NamedStoreHandlerCompiler::value() { return StoreIC::ValueRegister(); }


#undef __
#define __ ACCESS_MASM(masm)


void NamedLoadHandlerCompiler::GenerateLoadViaGetter(
    MacroAssembler* masm, Handle<HeapType> type, Register receiver,
    Handle<JSFunction> getter) {
  // ----------- S t a t e -------------
  //  -- r3    : receiver
  //  -- r5    : name
  //  -- lr    : return address
  // -----------------------------------
  {
    FrameAndConstantPoolScope scope(masm, StackFrame::INTERNAL);

    if (!getter.is_null()) {
      // Call the JavaScript getter with the receiver on the stack.
      if (IC::TypeToMap(*type, masm->isolate())->IsJSGlobalObjectMap()) {
        // Swap in the global receiver.
        __ LoadP(receiver,
                 FieldMemOperand(receiver, JSGlobalObject::kGlobalProxyOffset));
      }
      __ push(receiver);
      ParameterCount actual(0);
      ParameterCount expected(getter);
      __ InvokeFunction(getter, expected, actual,
                        CALL_FUNCTION, NullCallWrapper());
    } else {
      // If we generate a global code snippet for deoptimization only, remember
      // the place to continue after deoptimization.
      masm->isolate()->heap()->SetGetterStubDeoptPCOffset(masm->pc_offset());
    }

    // Restore context register.
    __ LoadP(cp, MemOperand(fp, StandardFrameConstants::kContextOffset));
  }
  __ Ret();
}


#undef __
#define __ ACCESS_MASM(masm())


Handle<Code> NamedLoadHandlerCompiler::CompileLoadGlobal(
    Handle<PropertyCell> cell, Handle<Name> name, bool is_configurable) {
  Label miss;
  FrontendHeader(receiver(), name, &miss);

  // Get the value from the cell.
  Register result = StoreIC::ValueRegister();
  __ mov(result, Operand(cell));
  __ LoadP(result, FieldMemOperand(result, Cell::kValueOffset));

  // Check for deleted property if property can actually be deleted.
  if (is_configurable) {
    __ LoadRoot(ip, Heap::kTheHoleValueRootIndex);
    __ cmp(result, ip);
    __ beq(&miss);
  }

  Counters* counters = isolate()->counters();
  __ IncrementCounter(counters->named_load_global_stub(), 1, r4, r6);
  __ Ret();

  FrontendFooter(name, &miss);

  // Return the generated code.
  return GetCode(kind(), Code::NORMAL, name);
}


Handle<Code> PropertyICCompiler::CompilePolymorphic(TypeHandleList* types,
                                                    CodeHandleList* handlers,
                                                    Handle<Name> name,
                                                    Code::StubType type,
                                                    IcCheckType check) {
  Label miss;

  if (check == PROPERTY &&
      (kind() == Code::KEYED_LOAD_IC || kind() == Code::KEYED_STORE_IC)) {
    // In case we are compiling an IC for dictionary loads and stores, just
    // check whether the name is unique.
    if (name.is_identical_to(isolate()->factory()->normal_ic_symbol())) {
      __ JumpIfNotUniqueName(this->name(), &miss);
    } else {
      __ Cmpi(this->name(), Operand(name), r0);
      __ bne(&miss);
    }
  }

  Label number_case;
  Label* smi_target = IncludesNumberType(types) ? &number_case : &miss;
  __ JumpIfSmi(receiver(), smi_target);

  // Polymorphic keyed stores may use the map register
  Register map_reg = scratch1();
  DCHECK(kind() != Code::KEYED_STORE_IC ||
         map_reg.is(KeyedStoreIC::MapRegister()));

  int receiver_count = types->length();
  int number_of_handled_maps = 0;
  __ LoadP(map_reg, FieldMemOperand(receiver(), HeapObject::kMapOffset));
  for (int current = 0; current < receiver_count; ++current) {
    Handle<HeapType> type = types->at(current);
    Handle<Map> map = IC::TypeToMap(*type, isolate());
    if (!map->is_deprecated()) {
      number_of_handled_maps++;
      __ mov(ip, Operand(map));
      __ cmp(map_reg, ip);
      if (type->Is(HeapType::Number())) {
        DCHECK(!number_case.is_unused());
        __ bind(&number_case);
      }
      __ Jump(handlers->at(current), RelocInfo::CODE_TARGET, eq);
    }
  }
  DCHECK(number_of_handled_maps != 0);

  __ bind(&miss);
  TailCallBuiltin(masm(), MissBuiltin(kind()));

  // Return the generated code.
  InlineCacheState state =
      number_of_handled_maps > 1 ? POLYMORPHIC : MONOMORPHIC;
  return GetCode(kind(), type, name, state);
}


Handle<Code> PropertyICCompiler::CompileKeyedStorePolymorphic(
    MapHandleList* receiver_maps, CodeHandleList* handler_stubs,
    MapHandleList* transitioned_maps) {
  Label miss;
  __ JumpIfSmi(receiver(), &miss);

  int receiver_count = receiver_maps->length();
  __ LoadP(scratch1(), FieldMemOperand(receiver(), HeapObject::kMapOffset));
  for (int i = 0; i < receiver_count; ++i) {
    __ mov(ip, Operand(receiver_maps->at(i)));
    __ cmp(scratch1(), ip);
    if (transitioned_maps->at(i).is_null()) {
      Label skip;
      __ bne(&skip);
      __ Jump(handler_stubs->at(i), RelocInfo::CODE_TARGET);
      __ bind(&skip);
    } else {
      Label next_map;
      __ bne(&next_map);
      __ mov(transition_map(), Operand(transitioned_maps->at(i)));
      __ Jump(handler_stubs->at(i), RelocInfo::CODE_TARGET, al);
      __ bind(&next_map);
    }
  }

  __ bind(&miss);
  TailCallBuiltin(masm(), MissBuiltin(kind()));

  // Return the generated code.
  return GetCode(kind(), Code::NORMAL, factory()->empty_string(), POLYMORPHIC);
}


#undef __
#define __ ACCESS_MASM(masm)


void ElementHandlerCompiler::GenerateLoadDictionaryElement(
    MacroAssembler* masm) {
  // The return address is in lr.
  Label slow, miss;

  Register key = LoadIC::NameRegister();
  Register receiver = LoadIC::ReceiverRegister();
  DCHECK(receiver.is(r4));
  DCHECK(key.is(r5));

  __ UntagAndJumpIfNotSmi(r9, key, &miss);
  __ LoadP(r7, FieldMemOperand(receiver, JSObject::kElementsOffset));
  __ LoadFromNumberDictionary(&slow, r7, key, r3, r9, r6, r8);
  __ Ret();

  __ bind(&slow);
  __ IncrementCounter(
      masm->isolate()->counters()->keyed_load_external_array_slow(),
      1, r5, r6);

  TailCallBuiltin(masm, Builtins::kKeyedLoadIC_Slow);

  // Miss case, call the runtime.
  __ bind(&miss);

  TailCallBuiltin(masm, Builtins::kKeyedLoadIC_Miss);
}


#undef __

} }  // namespace v8::internal

#endif  // V8_TARGET_ARCH_PPC
