/* Copyright (C) 2002-2004 RealVNC Ltd.  All Rights Reserved.
 *    
 * This is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 * 
 * This software is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this software; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307,
 * USA.
 */

// -=- Configuration.h
//
// This header defines a set of classes used to represent configuration
// parameters of different types.  Instances of the different parameter
// types are associated with instances of the Configuration class, and
// are each given a unique name.  The Configuration class provides a
// generic API through which parameters may be located by name and their
// value set, thus removing the need to write platform-specific code.
// Simply defining a new parameter and associating it with a Configuration
// will allow it to be configured by the user.
//
// The Configuration class is used to allow multiple distinct configurations
// to co-exist at the same time.  A process serving several desktops, for
// instance, can create a Configuration instance for each, to allow them
// to be configured independently, from the command-line, registry, etc.

#ifndef __RFB_CONFIGURATION_H__
#define __RFB_CONFIGURATION_H__

namespace rfb {
  class VoidParameter;

  // -=- Configuration
  //     Class used to access parameters.

  class Configuration {
  public:
    // - Set named parameter to value
    static bool setParam(const char* param, const char* value, bool immutable=false);

    // - Set parameter to value (separated by "=")
    static bool setParam(const char* config, bool immutable=false);

    // - Set named parameter to value, with name truncated at len
    static bool setParam(const char* name, int len,
                         const char* val, bool immutable);

    // - Get named parameter
    static VoidParameter* getParam(const char* param);

    static void listParams(int width=79, int nameWidth=10);

    static VoidParameter* head;
  };

  // -=- VoidParameter
  //     Configuration parameter base-class.

  class VoidParameter {
  public:
    VoidParameter(const char* name_, const char* desc_);
    virtual  ~VoidParameter();
    const char* getName() const;
    const char* getDescription() const;

    virtual bool setParam(const char* value)  = 0;
    virtual bool setParam();
    virtual char* getDefaultStr() const = 0;
    virtual char* getValueStr() const = 0;
    virtual bool isBool() const;

    virtual void setImmutable();
    virtual void setHasBeenSet();
    bool hasBeenSet();

    VoidParameter* _next;
  protected:
    bool immutable;
    bool _hasBeenSet;
    const char* name;
    const char* description;
  };

  class AliasParameter : public VoidParameter {
  public:
    AliasParameter(const char* name_, const char* desc_,VoidParameter* param_);
    virtual bool setParam(const char* value);
    virtual bool setParam();
    virtual char* getDefaultStr() const;
    virtual char* getValueStr() const;
    virtual bool isBool() const;
  private:
    VoidParameter* param;
  };

  class BoolParameter : public VoidParameter {
  public:
    BoolParameter(const char* name_, const char* desc_, bool v);
    virtual bool setParam(const char* value);
    virtual bool setParam();
    virtual void setParam(bool b);
    virtual char* getDefaultStr() const;
    virtual char* getValueStr() const;
    virtual bool isBool() const;
    operator bool() const;
  protected:
    bool value;
    bool def_value;
  };

  class IntParameter : public VoidParameter {
  public:
    IntParameter(const char* name_, const char* desc_, int v);
    virtual bool setParam(const char* value);
    virtual bool setParam(int v);
    virtual char* getDefaultStr() const;
    virtual char* getValueStr() const;
    operator int() const;
  protected:
    int value;
    int def_value;
  };

  class StringParameter : public VoidParameter {
  public:
    // StringParameter contains a null-terminated string, which CANNOT
    // be Null, and so neither can the default value!
    StringParameter(const char* name_, const char* desc_, const char* v);
    virtual ~StringParameter();
    virtual bool setParam(const char* value);
    virtual char* getDefaultStr() const;
    virtual char* getValueStr() const;

    // getData() returns a copy of the data - it must be delete[]d by the
    // caller.
    char* getData() const { return getValueStr(); }
  protected:
    char* value;
    const char* def_value;
  };

  class BinaryParameter : public VoidParameter {
  public:
    BinaryParameter(const char* name_, const char* desc_, const void* v, int l);
    virtual ~BinaryParameter();
    virtual bool setParam(const char* value);
    virtual void setParam(const void* v, int l);
    virtual char* getDefaultStr() const;
    virtual char* getValueStr() const;

    void getData(void** data, int* length) const;

  protected:
    char* value;
    int length;
    char* def_value;
    int def_length;
  };

};

#endif // __RFB_CONFIGURATION_H__
