/* Copyright (C) 2002-2005 RealVNC Ltd.  All Rights Reserved.
 * Copyright 2011-2022 Pierre Ossman for Cendio AB
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
// If no Configuration is specified when creating a Parameter, then the
// global Configuration will be assumed.
//
// Configurations can be "chained" into groups.  Each group has a root
// Configuration, a pointer to which should be passed to the constructors
// of the other group members.  set() and get() operations called on the
// root will iterate through all of the group's members.
//
// NB: On platforms that support Threading, locking is performed to protect
//     complex parameter types from concurrent access (e.g. strings).
// NB: NO LOCKING is performed when linking Configurations to groups
//     or when adding Parameters to Configurations.

#ifndef __CORE_CONFIGURATION_H__
#define __CORE_CONFIGURATION_H__

#include <limits.h>
#include <stdint.h>

#include <list>
#include <string>
#include <vector>

namespace core {

  class VoidParameter;

  // -=- Configuration
  //     Class used to access parameters.

  class Configuration {
  public:
    // - Create a new Configuration object
    Configuration() {}

    // - Set named parameter to value
    bool set(const char* param, const char* value, bool immutable=false);

    // - Get named parameter
    VoidParameter* get(const char* param);

    // - List the parameters of this Configuration group
    void list(int width=79, int nameWidth=10);

    // - Remove a parameter from this Configuration group
    bool remove(const char* param);

    // - handleArg
    //   Parse a command line argument into a parameter, returning how
    //   many arguments were consumed
    int handleArg(int argc, char* argv[], int index);


    // - Iterate over all parameters
    std::list<VoidParameter*>::iterator begin() { return params.begin(); }
    std::list<VoidParameter*>::iterator end() { return params.end(); }


    // - Get the Global Configuration object
    //   NB: This call does NOT lock the Configuration system.
    //       ALWAYS ensure that if you have ANY global Parameters,
    //       then they are defined as global objects, to ensure that
    //       global() is called when only the main thread is running.
    static Configuration* global();

    // - Container for process-wide Global parameters
    static bool setParam(const char* param, const char* value, bool immutable=false) {
      return global()->set(param, value, immutable);
    }
    static VoidParameter* getParam(const char* param) { return global()->get(param); }
    static void listParams(int width=79, int nameWidth=10) {
      global()->list(width, nameWidth);
    }
    static bool removeParam(const char* param) {
      return global()->remove(param);
    }
    static int handleParamArg(int argc, char* argv[], int index) {
      return global()->handleArg(argc, argv, index);
    }

  private:
    friend class VoidParameter;

    // - List of Parameters
    std::list<VoidParameter*> params;

    // The process-wide, Global Configuration object
    static Configuration* global_;
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
    virtual std::string getDefaultStr() const = 0;
    virtual std::string getValueStr() const = 0;

    virtual bool isDefault() const;

    virtual void setImmutable();

  protected:
    friend class Configuration;

    VoidParameter* _next;
    bool immutable;
    const char* name;
    const char* description;
  };

  class AliasParameter : public VoidParameter {
  public:
    AliasParameter(const char* name_, const char* desc_,VoidParameter* param_);
    bool setParam(const char* value) override;
    bool setParam() override;
    std::string getDefaultStr() const override;
    std::string getValueStr() const override;
    void setImmutable() override;
  private:
    VoidParameter* param;
  };

  class BoolParameter : public VoidParameter {
  public:
    BoolParameter(const char* name_, const char* desc_, bool v);
    bool setParam(const char* value) override;
    bool setParam() override;
    virtual void setParam(bool b);
    std::string getDefaultStr() const override;
    std::string getValueStr() const override;
    operator bool() const;
  protected:
    bool value;
    bool def_value;
  };

  class IntParameter : public VoidParameter {
  public:
    IntParameter(const char* name_, const char* desc_, int v,
                 int minValue=INT_MIN, int maxValue=INT_MAX);
    using VoidParameter::setParam;
    bool setParam(const char* value) override;
    virtual bool setParam(int v);
    std::string getDefaultStr() const override;
    std::string getValueStr() const override;
    operator int() const;
  protected:
    int value;
    int def_value;
    int minValue, maxValue;
  };

  class StringParameter : public VoidParameter {
  public:
    StringParameter(const char* name_, const char* desc_, const char* v);
    bool setParam(const char* value) override;
    std::string getDefaultStr() const override;
    std::string getValueStr() const override;
    operator const char*() const;
  protected:
    std::string value;
    std::string def_value;
  };

  class BinaryParameter : public VoidParameter {
  public:
    BinaryParameter(const char* name_, const char* desc_,
                    const uint8_t* v, size_t l);
    using VoidParameter::setParam;
    ~BinaryParameter() override;
    bool setParam(const char* value) override;
    virtual void setParam(const uint8_t* v, size_t l);
    std::string getDefaultStr() const override;
    std::string getValueStr() const override;

    std::vector<uint8_t> getData() const;

  protected:
    uint8_t* value;
    size_t length;
    uint8_t* def_value;
    size_t def_length;
  };

};

#endif // __CORE_CONFIGURATION_H__
