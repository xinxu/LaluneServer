// Generated by the protocol buffer compiler.  DO NOT EDIT!
// source: commonlib/CommonLib.proto

#ifndef PROTOBUF_commonlib_2fCommonLib_2eproto__INCLUDED
#define PROTOBUF_commonlib_2fCommonLib_2eproto__INCLUDED

#include <string>

#include <google/protobuf/stubs/common.h>

#if GOOGLE_PROTOBUF_VERSION < 2006000
#error This file was generated by a newer version of protoc which is
#error incompatible with your Protocol Buffer headers.  Please update
#error your headers.
#endif
#if 2006000 < GOOGLE_PROTOBUF_MIN_PROTOC_VERSION
#error This file was generated by an older version of protoc which is
#error incompatible with your Protocol Buffer headers.  Please
#error regenerate this file with a newer version of protoc.
#endif

#include <google/protobuf/generated_message_util.h>
#include <google/protobuf/message.h>
#include <google/protobuf/repeated_field.h>
#include <google/protobuf/extension_set.h>
#include <google/protobuf/unknown_field_set.h>
// @@protoc_insertion_point(includes)

namespace common {

// Internal implementation detail -- do not call these.
void  protobuf_AddDesc_commonlib_2fCommonLib_2eproto();
void protobuf_AssignDesc_commonlib_2fCommonLib_2eproto();
void protobuf_ShutdownFile_commonlib_2fCommonLib_2eproto();

class HeaderEx;
class Hello;
class ReportLoad;
class AddressInfo;
class AddressList;
class ServerId;

// ===================================================================

class HeaderEx : public ::google_lalune::protobuf::Message {
 public:
  HeaderEx();
  virtual ~HeaderEx();

  HeaderEx(const HeaderEx& from);

  inline HeaderEx& operator=(const HeaderEx& from) {
    CopyFrom(from);
    return *this;
  }

  inline const ::google_lalune::protobuf::UnknownFieldSet& unknown_fields() const {
    return _unknown_fields_;
  }

  inline ::google_lalune::protobuf::UnknownFieldSet* mutable_unknown_fields() {
    return &_unknown_fields_;
  }

  static const ::google_lalune::protobuf::Descriptor* descriptor();
  static const HeaderEx& default_instance();

  void Swap(HeaderEx* other);

  // implements Message ----------------------------------------------

  HeaderEx* New() const;
  void CopyFrom(const ::google_lalune::protobuf::Message& from);
  void MergeFrom(const ::google_lalune::protobuf::Message& from);
  void CopyFrom(const HeaderEx& from);
  void MergeFrom(const HeaderEx& from);
  void Clear();
  bool IsInitialized() const;

  int ByteSize() const;
  bool MergePartialFromCodedStream(
      ::google_lalune::protobuf::io::CodedInputStream* input);
  void SerializeWithCachedSizes(
      ::google_lalune::protobuf::io::CodedOutputStream* output) const;
  ::google_lalune::protobuf::uint8* SerializeWithCachedSizesToArray(::google_lalune::protobuf::uint8* output) const;
  int GetCachedSize() const { return _cached_size_; }
  private:
  void SharedCtor();
  void SharedDtor();
  void SetCachedSize(int size) const;
  public:
  ::google_lalune::protobuf::Metadata GetMetadata() const;

  // nested types ----------------------------------------------------

  // accessors -------------------------------------------------------

  // optional uint64 uid = 1;
  inline bool has_uid() const;
  inline void clear_uid();
  static const int kUidFieldNumber = 1;
  inline ::google_lalune::protobuf::uint64 uid() const;
  inline void set_uid(::google_lalune::protobuf::uint64 value);

  // repeated uint32 corresponding_servers = 2;
  inline int corresponding_servers_size() const;
  inline void clear_corresponding_servers();
  static const int kCorrespondingServersFieldNumber = 2;
  inline ::google_lalune::protobuf::uint32 corresponding_servers(int index) const;
  inline void set_corresponding_servers(int index, ::google_lalune::protobuf::uint32 value);
  inline void add_corresponding_servers(::google_lalune::protobuf::uint32 value);
  inline const ::google_lalune::protobuf::RepeatedField< ::google_lalune::protobuf::uint32 >&
      corresponding_servers() const;
  inline ::google_lalune::protobuf::RepeatedField< ::google_lalune::protobuf::uint32 >*
      mutable_corresponding_servers();

  // @@protoc_insertion_point(class_scope:common.HeaderEx)
 private:
  inline void set_has_uid();
  inline void clear_has_uid();

  ::google_lalune::protobuf::UnknownFieldSet _unknown_fields_;

  ::google_lalune::protobuf::uint32 _has_bits_[1];
  mutable int _cached_size_;
  ::google_lalune::protobuf::uint64 uid_;
  ::google_lalune::protobuf::RepeatedField< ::google_lalune::protobuf::uint32 > corresponding_servers_;
  friend void  protobuf_AddDesc_commonlib_2fCommonLib_2eproto();
  friend void protobuf_AssignDesc_commonlib_2fCommonLib_2eproto();
  friend void protobuf_ShutdownFile_commonlib_2fCommonLib_2eproto();

  void InitAsDefaultInstance();
  static HeaderEx* default_instance_;
};
// -------------------------------------------------------------------

class Hello : public ::google_lalune::protobuf::Message {
 public:
  Hello();
  virtual ~Hello();

  Hello(const Hello& from);

  inline Hello& operator=(const Hello& from) {
    CopyFrom(from);
    return *this;
  }

  inline const ::google_lalune::protobuf::UnknownFieldSet& unknown_fields() const {
    return _unknown_fields_;
  }

  inline ::google_lalune::protobuf::UnknownFieldSet* mutable_unknown_fields() {
    return &_unknown_fields_;
  }

  static const ::google_lalune::protobuf::Descriptor* descriptor();
  static const Hello& default_instance();

  void Swap(Hello* other);

  // implements Message ----------------------------------------------

  Hello* New() const;
  void CopyFrom(const ::google_lalune::protobuf::Message& from);
  void MergeFrom(const ::google_lalune::protobuf::Message& from);
  void CopyFrom(const Hello& from);
  void MergeFrom(const Hello& from);
  void Clear();
  bool IsInitialized() const;

  int ByteSize() const;
  bool MergePartialFromCodedStream(
      ::google_lalune::protobuf::io::CodedInputStream* input);
  void SerializeWithCachedSizes(
      ::google_lalune::protobuf::io::CodedOutputStream* output) const;
  ::google_lalune::protobuf::uint8* SerializeWithCachedSizesToArray(::google_lalune::protobuf::uint8* output) const;
  int GetCachedSize() const { return _cached_size_; }
  private:
  void SharedCtor();
  void SharedDtor();
  void SetCachedSize(int size) const;
  public:
  ::google_lalune::protobuf::Metadata GetMetadata() const;

  // nested types ----------------------------------------------------

  // accessors -------------------------------------------------------

  // required uint32 my_listening_port = 1;
  inline bool has_my_listening_port() const;
  inline void clear_my_listening_port();
  static const int kMyListeningPortFieldNumber = 1;
  inline ::google_lalune::protobuf::uint32 my_listening_port() const;
  inline void set_my_listening_port(::google_lalune::protobuf::uint32 value);

  // required uint32 server_type = 2;
  inline bool has_server_type() const;
  inline void clear_server_type();
  static const int kServerTypeFieldNumber = 2;
  inline ::google_lalune::protobuf::uint32 server_type() const;
  inline void set_server_type(::google_lalune::protobuf::uint32 value);

  // required uint32 is_server_start = 3;
  inline bool has_is_server_start() const;
  inline void clear_is_server_start();
  static const int kIsServerStartFieldNumber = 3;
  inline ::google_lalune::protobuf::uint32 is_server_start() const;
  inline void set_is_server_start(::google_lalune::protobuf::uint32 value);

  // @@protoc_insertion_point(class_scope:common.Hello)
 private:
  inline void set_has_my_listening_port();
  inline void clear_has_my_listening_port();
  inline void set_has_server_type();
  inline void clear_has_server_type();
  inline void set_has_is_server_start();
  inline void clear_has_is_server_start();

  ::google_lalune::protobuf::UnknownFieldSet _unknown_fields_;

  ::google_lalune::protobuf::uint32 _has_bits_[1];
  mutable int _cached_size_;
  ::google_lalune::protobuf::uint32 my_listening_port_;
  ::google_lalune::protobuf::uint32 server_type_;
  ::google_lalune::protobuf::uint32 is_server_start_;
  friend void  protobuf_AddDesc_commonlib_2fCommonLib_2eproto();
  friend void protobuf_AssignDesc_commonlib_2fCommonLib_2eproto();
  friend void protobuf_ShutdownFile_commonlib_2fCommonLib_2eproto();

  void InitAsDefaultInstance();
  static Hello* default_instance_;
};
// -------------------------------------------------------------------

class ReportLoad : public ::google_lalune::protobuf::Message {
 public:
  ReportLoad();
  virtual ~ReportLoad();

  ReportLoad(const ReportLoad& from);

  inline ReportLoad& operator=(const ReportLoad& from) {
    CopyFrom(from);
    return *this;
  }

  inline const ::google_lalune::protobuf::UnknownFieldSet& unknown_fields() const {
    return _unknown_fields_;
  }

  inline ::google_lalune::protobuf::UnknownFieldSet* mutable_unknown_fields() {
    return &_unknown_fields_;
  }

  static const ::google_lalune::protobuf::Descriptor* descriptor();
  static const ReportLoad& default_instance();

  void Swap(ReportLoad* other);

  // implements Message ----------------------------------------------

  ReportLoad* New() const;
  void CopyFrom(const ::google_lalune::protobuf::Message& from);
  void MergeFrom(const ::google_lalune::protobuf::Message& from);
  void CopyFrom(const ReportLoad& from);
  void MergeFrom(const ReportLoad& from);
  void Clear();
  bool IsInitialized() const;

  int ByteSize() const;
  bool MergePartialFromCodedStream(
      ::google_lalune::protobuf::io::CodedInputStream* input);
  void SerializeWithCachedSizes(
      ::google_lalune::protobuf::io::CodedOutputStream* output) const;
  ::google_lalune::protobuf::uint8* SerializeWithCachedSizesToArray(::google_lalune::protobuf::uint8* output) const;
  int GetCachedSize() const { return _cached_size_; }
  private:
  void SharedCtor();
  void SharedDtor();
  void SetCachedSize(int size) const;
  public:
  ::google_lalune::protobuf::Metadata GetMetadata() const;

  // nested types ----------------------------------------------------

  // accessors -------------------------------------------------------

  // required float load = 1;
  inline bool has_load() const;
  inline void clear_load();
  static const int kLoadFieldNumber = 1;
  inline float load() const;
  inline void set_load(float value);

  // @@protoc_insertion_point(class_scope:common.ReportLoad)
 private:
  inline void set_has_load();
  inline void clear_has_load();

  ::google_lalune::protobuf::UnknownFieldSet _unknown_fields_;

  ::google_lalune::protobuf::uint32 _has_bits_[1];
  mutable int _cached_size_;
  float load_;
  friend void  protobuf_AddDesc_commonlib_2fCommonLib_2eproto();
  friend void protobuf_AssignDesc_commonlib_2fCommonLib_2eproto();
  friend void protobuf_ShutdownFile_commonlib_2fCommonLib_2eproto();

  void InitAsDefaultInstance();
  static ReportLoad* default_instance_;
};
// -------------------------------------------------------------------

class AddressInfo : public ::google_lalune::protobuf::Message {
 public:
  AddressInfo();
  virtual ~AddressInfo();

  AddressInfo(const AddressInfo& from);

  inline AddressInfo& operator=(const AddressInfo& from) {
    CopyFrom(from);
    return *this;
  }

  inline const ::google_lalune::protobuf::UnknownFieldSet& unknown_fields() const {
    return _unknown_fields_;
  }

  inline ::google_lalune::protobuf::UnknownFieldSet* mutable_unknown_fields() {
    return &_unknown_fields_;
  }

  static const ::google_lalune::protobuf::Descriptor* descriptor();
  static const AddressInfo& default_instance();

  void Swap(AddressInfo* other);

  // implements Message ----------------------------------------------

  AddressInfo* New() const;
  void CopyFrom(const ::google_lalune::protobuf::Message& from);
  void MergeFrom(const ::google_lalune::protobuf::Message& from);
  void CopyFrom(const AddressInfo& from);
  void MergeFrom(const AddressInfo& from);
  void Clear();
  bool IsInitialized() const;

  int ByteSize() const;
  bool MergePartialFromCodedStream(
      ::google_lalune::protobuf::io::CodedInputStream* input);
  void SerializeWithCachedSizes(
      ::google_lalune::protobuf::io::CodedOutputStream* output) const;
  ::google_lalune::protobuf::uint8* SerializeWithCachedSizesToArray(::google_lalune::protobuf::uint8* output) const;
  int GetCachedSize() const { return _cached_size_; }
  private:
  void SharedCtor();
  void SharedDtor();
  void SetCachedSize(int size) const;
  public:
  ::google_lalune::protobuf::Metadata GetMetadata() const;

  // nested types ----------------------------------------------------

  // accessors -------------------------------------------------------

  // required fixed32 ip = 1;
  inline bool has_ip() const;
  inline void clear_ip();
  static const int kIpFieldNumber = 1;
  inline ::google_lalune::protobuf::uint32 ip() const;
  inline void set_ip(::google_lalune::protobuf::uint32 value);

  // required uint32 port = 2;
  inline bool has_port() const;
  inline void clear_port();
  static const int kPortFieldNumber = 2;
  inline ::google_lalune::protobuf::uint32 port() const;
  inline void set_port(::google_lalune::protobuf::uint32 value);

  // required uint32 server_id = 3;
  inline bool has_server_id() const;
  inline void clear_server_id();
  static const int kServerIdFieldNumber = 3;
  inline ::google_lalune::protobuf::uint32 server_id() const;
  inline void set_server_id(::google_lalune::protobuf::uint32 value);

  // required uint32 server_type = 4;
  inline bool has_server_type() const;
  inline void clear_server_type();
  static const int kServerTypeFieldNumber = 4;
  inline ::google_lalune::protobuf::uint32 server_type() const;
  inline void set_server_type(::google_lalune::protobuf::uint32 value);

  // @@protoc_insertion_point(class_scope:common.AddressInfo)
 private:
  inline void set_has_ip();
  inline void clear_has_ip();
  inline void set_has_port();
  inline void clear_has_port();
  inline void set_has_server_id();
  inline void clear_has_server_id();
  inline void set_has_server_type();
  inline void clear_has_server_type();

  ::google_lalune::protobuf::UnknownFieldSet _unknown_fields_;

  ::google_lalune::protobuf::uint32 _has_bits_[1];
  mutable int _cached_size_;
  ::google_lalune::protobuf::uint32 ip_;
  ::google_lalune::protobuf::uint32 port_;
  ::google_lalune::protobuf::uint32 server_id_;
  ::google_lalune::protobuf::uint32 server_type_;
  friend void  protobuf_AddDesc_commonlib_2fCommonLib_2eproto();
  friend void protobuf_AssignDesc_commonlib_2fCommonLib_2eproto();
  friend void protobuf_ShutdownFile_commonlib_2fCommonLib_2eproto();

  void InitAsDefaultInstance();
  static AddressInfo* default_instance_;
};
// -------------------------------------------------------------------

class AddressList : public ::google_lalune::protobuf::Message {
 public:
  AddressList();
  virtual ~AddressList();

  AddressList(const AddressList& from);

  inline AddressList& operator=(const AddressList& from) {
    CopyFrom(from);
    return *this;
  }

  inline const ::google_lalune::protobuf::UnknownFieldSet& unknown_fields() const {
    return _unknown_fields_;
  }

  inline ::google_lalune::protobuf::UnknownFieldSet* mutable_unknown_fields() {
    return &_unknown_fields_;
  }

  static const ::google_lalune::protobuf::Descriptor* descriptor();
  static const AddressList& default_instance();

  void Swap(AddressList* other);

  // implements Message ----------------------------------------------

  AddressList* New() const;
  void CopyFrom(const ::google_lalune::protobuf::Message& from);
  void MergeFrom(const ::google_lalune::protobuf::Message& from);
  void CopyFrom(const AddressList& from);
  void MergeFrom(const AddressList& from);
  void Clear();
  bool IsInitialized() const;

  int ByteSize() const;
  bool MergePartialFromCodedStream(
      ::google_lalune::protobuf::io::CodedInputStream* input);
  void SerializeWithCachedSizes(
      ::google_lalune::protobuf::io::CodedOutputStream* output) const;
  ::google_lalune::protobuf::uint8* SerializeWithCachedSizesToArray(::google_lalune::protobuf::uint8* output) const;
  int GetCachedSize() const { return _cached_size_; }
  private:
  void SharedCtor();
  void SharedDtor();
  void SetCachedSize(int size) const;
  public:
  ::google_lalune::protobuf::Metadata GetMetadata() const;

  // nested types ----------------------------------------------------

  // accessors -------------------------------------------------------

  // repeated .common.AddressInfo addr = 1;
  inline int addr_size() const;
  inline void clear_addr();
  static const int kAddrFieldNumber = 1;
  inline const ::common::AddressInfo& addr(int index) const;
  inline ::common::AddressInfo* mutable_addr(int index);
  inline ::common::AddressInfo* add_addr();
  inline const ::google_lalune::protobuf::RepeatedPtrField< ::common::AddressInfo >&
      addr() const;
  inline ::google_lalune::protobuf::RepeatedPtrField< ::common::AddressInfo >*
      mutable_addr();

  // @@protoc_insertion_point(class_scope:common.AddressList)
 private:

  ::google_lalune::protobuf::UnknownFieldSet _unknown_fields_;

  ::google_lalune::protobuf::uint32 _has_bits_[1];
  mutable int _cached_size_;
  ::google_lalune::protobuf::RepeatedPtrField< ::common::AddressInfo > addr_;
  friend void  protobuf_AddDesc_commonlib_2fCommonLib_2eproto();
  friend void protobuf_AssignDesc_commonlib_2fCommonLib_2eproto();
  friend void protobuf_ShutdownFile_commonlib_2fCommonLib_2eproto();

  void InitAsDefaultInstance();
  static AddressList* default_instance_;
};
// -------------------------------------------------------------------

class ServerId : public ::google_lalune::protobuf::Message {
 public:
  ServerId();
  virtual ~ServerId();

  ServerId(const ServerId& from);

  inline ServerId& operator=(const ServerId& from) {
    CopyFrom(from);
    return *this;
  }

  inline const ::google_lalune::protobuf::UnknownFieldSet& unknown_fields() const {
    return _unknown_fields_;
  }

  inline ::google_lalune::protobuf::UnknownFieldSet* mutable_unknown_fields() {
    return &_unknown_fields_;
  }

  static const ::google_lalune::protobuf::Descriptor* descriptor();
  static const ServerId& default_instance();

  void Swap(ServerId* other);

  // implements Message ----------------------------------------------

  ServerId* New() const;
  void CopyFrom(const ::google_lalune::protobuf::Message& from);
  void MergeFrom(const ::google_lalune::protobuf::Message& from);
  void CopyFrom(const ServerId& from);
  void MergeFrom(const ServerId& from);
  void Clear();
  bool IsInitialized() const;

  int ByteSize() const;
  bool MergePartialFromCodedStream(
      ::google_lalune::protobuf::io::CodedInputStream* input);
  void SerializeWithCachedSizes(
      ::google_lalune::protobuf::io::CodedOutputStream* output) const;
  ::google_lalune::protobuf::uint8* SerializeWithCachedSizesToArray(::google_lalune::protobuf::uint8* output) const;
  int GetCachedSize() const { return _cached_size_; }
  private:
  void SharedCtor();
  void SharedDtor();
  void SetCachedSize(int size) const;
  public:
  ::google_lalune::protobuf::Metadata GetMetadata() const;

  // nested types ----------------------------------------------------

  // accessors -------------------------------------------------------

  // required uint32 server_id = 1;
  inline bool has_server_id() const;
  inline void clear_server_id();
  static const int kServerIdFieldNumber = 1;
  inline ::google_lalune::protobuf::uint32 server_id() const;
  inline void set_server_id(::google_lalune::protobuf::uint32 value);

  // @@protoc_insertion_point(class_scope:common.ServerId)
 private:
  inline void set_has_server_id();
  inline void clear_has_server_id();

  ::google_lalune::protobuf::UnknownFieldSet _unknown_fields_;

  ::google_lalune::protobuf::uint32 _has_bits_[1];
  mutable int _cached_size_;
  ::google_lalune::protobuf::uint32 server_id_;
  friend void  protobuf_AddDesc_commonlib_2fCommonLib_2eproto();
  friend void protobuf_AssignDesc_commonlib_2fCommonLib_2eproto();
  friend void protobuf_ShutdownFile_commonlib_2fCommonLib_2eproto();

  void InitAsDefaultInstance();
  static ServerId* default_instance_;
};
// ===================================================================


// ===================================================================

// HeaderEx

// optional uint64 uid = 1;
inline bool HeaderEx::has_uid() const {
  return (_has_bits_[0] & 0x00000001u) != 0;
}
inline void HeaderEx::set_has_uid() {
  _has_bits_[0] |= 0x00000001u;
}
inline void HeaderEx::clear_has_uid() {
  _has_bits_[0] &= ~0x00000001u;
}
inline void HeaderEx::clear_uid() {
  uid_ = GOOGLE_ULONGLONG(0);
  clear_has_uid();
}
inline ::google_lalune::protobuf::uint64 HeaderEx::uid() const {
  // @@protoc_insertion_point(field_get:common.HeaderEx.uid)
  return uid_;
}
inline void HeaderEx::set_uid(::google_lalune::protobuf::uint64 value) {
  set_has_uid();
  uid_ = value;
  // @@protoc_insertion_point(field_set:common.HeaderEx.uid)
}

// repeated uint32 corresponding_servers = 2;
inline int HeaderEx::corresponding_servers_size() const {
  return corresponding_servers_.size();
}
inline void HeaderEx::clear_corresponding_servers() {
  corresponding_servers_.Clear();
}
inline ::google_lalune::protobuf::uint32 HeaderEx::corresponding_servers(int index) const {
  // @@protoc_insertion_point(field_get:common.HeaderEx.corresponding_servers)
  return corresponding_servers_.Get(index);
}
inline void HeaderEx::set_corresponding_servers(int index, ::google_lalune::protobuf::uint32 value) {
  corresponding_servers_.Set(index, value);
  // @@protoc_insertion_point(field_set:common.HeaderEx.corresponding_servers)
}
inline void HeaderEx::add_corresponding_servers(::google_lalune::protobuf::uint32 value) {
  corresponding_servers_.Add(value);
  // @@protoc_insertion_point(field_add:common.HeaderEx.corresponding_servers)
}
inline const ::google_lalune::protobuf::RepeatedField< ::google_lalune::protobuf::uint32 >&
HeaderEx::corresponding_servers() const {
  // @@protoc_insertion_point(field_list:common.HeaderEx.corresponding_servers)
  return corresponding_servers_;
}
inline ::google_lalune::protobuf::RepeatedField< ::google_lalune::protobuf::uint32 >*
HeaderEx::mutable_corresponding_servers() {
  // @@protoc_insertion_point(field_mutable_list:common.HeaderEx.corresponding_servers)
  return &corresponding_servers_;
}

// -------------------------------------------------------------------

// Hello

// required uint32 my_listening_port = 1;
inline bool Hello::has_my_listening_port() const {
  return (_has_bits_[0] & 0x00000001u) != 0;
}
inline void Hello::set_has_my_listening_port() {
  _has_bits_[0] |= 0x00000001u;
}
inline void Hello::clear_has_my_listening_port() {
  _has_bits_[0] &= ~0x00000001u;
}
inline void Hello::clear_my_listening_port() {
  my_listening_port_ = 0u;
  clear_has_my_listening_port();
}
inline ::google_lalune::protobuf::uint32 Hello::my_listening_port() const {
  // @@protoc_insertion_point(field_get:common.Hello.my_listening_port)
  return my_listening_port_;
}
inline void Hello::set_my_listening_port(::google_lalune::protobuf::uint32 value) {
  set_has_my_listening_port();
  my_listening_port_ = value;
  // @@protoc_insertion_point(field_set:common.Hello.my_listening_port)
}

// required uint32 server_type = 2;
inline bool Hello::has_server_type() const {
  return (_has_bits_[0] & 0x00000002u) != 0;
}
inline void Hello::set_has_server_type() {
  _has_bits_[0] |= 0x00000002u;
}
inline void Hello::clear_has_server_type() {
  _has_bits_[0] &= ~0x00000002u;
}
inline void Hello::clear_server_type() {
  server_type_ = 0u;
  clear_has_server_type();
}
inline ::google_lalune::protobuf::uint32 Hello::server_type() const {
  // @@protoc_insertion_point(field_get:common.Hello.server_type)
  return server_type_;
}
inline void Hello::set_server_type(::google_lalune::protobuf::uint32 value) {
  set_has_server_type();
  server_type_ = value;
  // @@protoc_insertion_point(field_set:common.Hello.server_type)
}

// required uint32 is_server_start = 3;
inline bool Hello::has_is_server_start() const {
  return (_has_bits_[0] & 0x00000004u) != 0;
}
inline void Hello::set_has_is_server_start() {
  _has_bits_[0] |= 0x00000004u;
}
inline void Hello::clear_has_is_server_start() {
  _has_bits_[0] &= ~0x00000004u;
}
inline void Hello::clear_is_server_start() {
  is_server_start_ = 0u;
  clear_has_is_server_start();
}
inline ::google_lalune::protobuf::uint32 Hello::is_server_start() const {
  // @@protoc_insertion_point(field_get:common.Hello.is_server_start)
  return is_server_start_;
}
inline void Hello::set_is_server_start(::google_lalune::protobuf::uint32 value) {
  set_has_is_server_start();
  is_server_start_ = value;
  // @@protoc_insertion_point(field_set:common.Hello.is_server_start)
}

// -------------------------------------------------------------------

// ReportLoad

// required float load = 1;
inline bool ReportLoad::has_load() const {
  return (_has_bits_[0] & 0x00000001u) != 0;
}
inline void ReportLoad::set_has_load() {
  _has_bits_[0] |= 0x00000001u;
}
inline void ReportLoad::clear_has_load() {
  _has_bits_[0] &= ~0x00000001u;
}
inline void ReportLoad::clear_load() {
  load_ = 0;
  clear_has_load();
}
inline float ReportLoad::load() const {
  // @@protoc_insertion_point(field_get:common.ReportLoad.load)
  return load_;
}
inline void ReportLoad::set_load(float value) {
  set_has_load();
  load_ = value;
  // @@protoc_insertion_point(field_set:common.ReportLoad.load)
}

// -------------------------------------------------------------------

// AddressInfo

// required fixed32 ip = 1;
inline bool AddressInfo::has_ip() const {
  return (_has_bits_[0] & 0x00000001u) != 0;
}
inline void AddressInfo::set_has_ip() {
  _has_bits_[0] |= 0x00000001u;
}
inline void AddressInfo::clear_has_ip() {
  _has_bits_[0] &= ~0x00000001u;
}
inline void AddressInfo::clear_ip() {
  ip_ = 0u;
  clear_has_ip();
}
inline ::google_lalune::protobuf::uint32 AddressInfo::ip() const {
  // @@protoc_insertion_point(field_get:common.AddressInfo.ip)
  return ip_;
}
inline void AddressInfo::set_ip(::google_lalune::protobuf::uint32 value) {
  set_has_ip();
  ip_ = value;
  // @@protoc_insertion_point(field_set:common.AddressInfo.ip)
}

// required uint32 port = 2;
inline bool AddressInfo::has_port() const {
  return (_has_bits_[0] & 0x00000002u) != 0;
}
inline void AddressInfo::set_has_port() {
  _has_bits_[0] |= 0x00000002u;
}
inline void AddressInfo::clear_has_port() {
  _has_bits_[0] &= ~0x00000002u;
}
inline void AddressInfo::clear_port() {
  port_ = 0u;
  clear_has_port();
}
inline ::google_lalune::protobuf::uint32 AddressInfo::port() const {
  // @@protoc_insertion_point(field_get:common.AddressInfo.port)
  return port_;
}
inline void AddressInfo::set_port(::google_lalune::protobuf::uint32 value) {
  set_has_port();
  port_ = value;
  // @@protoc_insertion_point(field_set:common.AddressInfo.port)
}

// required uint32 server_id = 3;
inline bool AddressInfo::has_server_id() const {
  return (_has_bits_[0] & 0x00000004u) != 0;
}
inline void AddressInfo::set_has_server_id() {
  _has_bits_[0] |= 0x00000004u;
}
inline void AddressInfo::clear_has_server_id() {
  _has_bits_[0] &= ~0x00000004u;
}
inline void AddressInfo::clear_server_id() {
  server_id_ = 0u;
  clear_has_server_id();
}
inline ::google_lalune::protobuf::uint32 AddressInfo::server_id() const {
  // @@protoc_insertion_point(field_get:common.AddressInfo.server_id)
  return server_id_;
}
inline void AddressInfo::set_server_id(::google_lalune::protobuf::uint32 value) {
  set_has_server_id();
  server_id_ = value;
  // @@protoc_insertion_point(field_set:common.AddressInfo.server_id)
}

// required uint32 server_type = 4;
inline bool AddressInfo::has_server_type() const {
  return (_has_bits_[0] & 0x00000008u) != 0;
}
inline void AddressInfo::set_has_server_type() {
  _has_bits_[0] |= 0x00000008u;
}
inline void AddressInfo::clear_has_server_type() {
  _has_bits_[0] &= ~0x00000008u;
}
inline void AddressInfo::clear_server_type() {
  server_type_ = 0u;
  clear_has_server_type();
}
inline ::google_lalune::protobuf::uint32 AddressInfo::server_type() const {
  // @@protoc_insertion_point(field_get:common.AddressInfo.server_type)
  return server_type_;
}
inline void AddressInfo::set_server_type(::google_lalune::protobuf::uint32 value) {
  set_has_server_type();
  server_type_ = value;
  // @@protoc_insertion_point(field_set:common.AddressInfo.server_type)
}

// -------------------------------------------------------------------

// AddressList

// repeated .common.AddressInfo addr = 1;
inline int AddressList::addr_size() const {
  return addr_.size();
}
inline void AddressList::clear_addr() {
  addr_.Clear();
}
inline const ::common::AddressInfo& AddressList::addr(int index) const {
  // @@protoc_insertion_point(field_get:common.AddressList.addr)
  return addr_.Get(index);
}
inline ::common::AddressInfo* AddressList::mutable_addr(int index) {
  // @@protoc_insertion_point(field_mutable:common.AddressList.addr)
  return addr_.Mutable(index);
}
inline ::common::AddressInfo* AddressList::add_addr() {
  // @@protoc_insertion_point(field_add:common.AddressList.addr)
  return addr_.Add();
}
inline const ::google_lalune::protobuf::RepeatedPtrField< ::common::AddressInfo >&
AddressList::addr() const {
  // @@protoc_insertion_point(field_list:common.AddressList.addr)
  return addr_;
}
inline ::google_lalune::protobuf::RepeatedPtrField< ::common::AddressInfo >*
AddressList::mutable_addr() {
  // @@protoc_insertion_point(field_mutable_list:common.AddressList.addr)
  return &addr_;
}

// -------------------------------------------------------------------

// ServerId

// required uint32 server_id = 1;
inline bool ServerId::has_server_id() const {
  return (_has_bits_[0] & 0x00000001u) != 0;
}
inline void ServerId::set_has_server_id() {
  _has_bits_[0] |= 0x00000001u;
}
inline void ServerId::clear_has_server_id() {
  _has_bits_[0] &= ~0x00000001u;
}
inline void ServerId::clear_server_id() {
  server_id_ = 0u;
  clear_has_server_id();
}
inline ::google_lalune::protobuf::uint32 ServerId::server_id() const {
  // @@protoc_insertion_point(field_get:common.ServerId.server_id)
  return server_id_;
}
inline void ServerId::set_server_id(::google_lalune::protobuf::uint32 value) {
  set_has_server_id();
  server_id_ = value;
  // @@protoc_insertion_point(field_set:common.ServerId.server_id)
}


// @@protoc_insertion_point(namespace_scope)

}  // namespace common

#ifndef SWIG
namespace google_lalune {
namespace protobuf {


}  // namespace google
}  // namespace protobuf
#endif  // SWIG

// @@protoc_insertion_point(global_scope)

#endif  // PROTOBUF_commonlib_2fCommonLib_2eproto__INCLUDED
