#!/usr/local/bin/python2.6 -tt
#
# Licensed to the Apache Software Foundation (ASF) under one
# or more contributor license agreements. See the NOTICE file
# distributed with this work for additional information
# regarding copyright ownership. The ASF licenses this file
# to you under the Apache License, Version 2.0 (the
# "License"); you may not use this file except in compliance
# with the License. You may obtain a copy of the License at
#
#   http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing,
# software distributed under the License is distributed on an
# "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
# KIND, either express or implied. See the License for the
# specific language governing permissions and limitations
# under the License.
#

import functools
import re
import weakref

from t_output import Output

class Primitive(object):
    '''
    Primitives are what we print to LogicalScopes.
    Bear in mind that we don't directly instantiate Primitives, but instead use
    the helper methods from an implementation of PrimitiveFactory to
    automatically do that.
    '''

    def __init__(self, parent, text, **kwargs):
        self._text = str(text)
        self._opts = kwargs
        self._scope = None
        self._opts['type'] = self.__class__
        # settings that get passed to the inner LogicalScope should one get
        # created
        self._parent = parent
        # for __setattr__ to work
        self._initialized = True

    def __getstate__(self):
        'Remove transitive data'
        persistent = ('_text', '_scope', '_parent', '_opts')
        return dict((attr, getattr(self, attr)) for attr in persistent)

    def __setstate__(self, state):
        "Don't set the context, we will use the one from the topmost scope"
        self.__dict__.update(state)
        self._initialized = True
        if self._scope:
            self._scope.enter_scope_callback = self.enter_scope_callback
            self._scope.exit_scope_callback = self.exit_scope_callback

    def __repr__(self):
        return 'Primitive({0}, {1})'.format(repr(self._text), repr(self._opts))

    def __str__(self):
        return self._text or ''

    def __setattr__(self, attr, value):
        # if it's set in the init method
        # or if it's an attribute that already exists
        if not '_initialized' in self.__dict__ or \
                attr in self.__dict__:
            self.__dict__[attr] = value
        # else set it to _opts
        else:
            self.__dict__['_opts'][attr] = value

    def __getattr__(self, attr):
        if attr.startswith('_'):
            # This shouldn't be forwarded to _opts
            raise AttributeError('Attribute {0} does not exist.'.format(attr))
        if attr in self._opts:
            return self._opts[attr]
        # Otherwise willingly return None. This is so that we can easily do
        # stuff like if somePrimitive.booleanAttr which may or may not be
        # defined
        #raise AttributeError('No option {0} in {1}'.format(attr, \
        #        repr(self)))

    def __contains__(self, attr):
        return attr in self._opts

    @property
    def parent(self):
        return self._parent

    @property
    def scope(self):
        'Lazily create singleton scope when asked for it'
        if self._scope is None:
            scope = LogicalScope(parent=self._parent,
                                 opts=self._opts)
            self._scope = scope
            scope.enter_scope_callback = self.enter_scope_callback
            scope.exit_scope_callback = self.exit_scope_callback
        return self._scope

    # call the scope's wrappers

    def __enter__(self):
        return self.scope.__enter__()

    def __exit__(self, *args):
        return self.scope.__exit__(args)

    def enter_scope_callback(self, context, scope):
        pass

    def exit_scope_callback(self, context, scope):
        pass

    def _write(self, context):
        '''Commit this primitive to a context. Only the LogicalScope to which
        this Primitive belongs to (i.e. the parent) should call this. '''
        raise NotImplementedError('Cannot write a base class Primitive')


class PrimitiveFactory:
    '''
    This makes it easy for generator code to create the necessary Primitives
    that make up the structure of a generated program. It provides helper
    functions that automatically create (and provide the scope of) all the
    needed primitives
    '''

    # It's the LogicalScope's (parent scope's) job to init us
    # We need to hold a weak reference to the parent scope, but dereference it
    # when we create a primitive
    def __init__(self, parent_scope_ref):
        self._scope = parent_scope_ref

        def generatePrimitive(type_, self, text, **kwargs):
            return type_(self._scope(), text, **kwargs)

        # generate all the type factories
        import new
        for fname, type_ in self.types.iteritems():
            func = functools.partial(generatePrimitive, type_)
            setattr(self, fname, new.instancemethod(func, self,
                PrimitiveFactory))


class Values(dict):
    def __init__(self, values):
        # internally reference the values
        self._values_store = values

    def __contains__(self, item):
        return item in self._values_store

    def __getattr__(self, item):
        if item in self:
            return self._values_store[item]
        else:
            raise AttributeError(repr(item) + \
                                 " not found in this Values object.")

    def __getstate__(self):
        return self._values_store

    def __setstate__(self, state):
        self._values_store = state


class LogicalScope:
    '''
    A scope that defines logical encapsulation of what is inside.  For
    instance, a namespace will contain a class which will contain members, i.e.
    methods and fields. "Contains" here refers to "*owns* a LogicalScope that
    contains".  Likewise, in any block of executable code, a loop or condition
    statement will own and be followed by a LogicalScope.
    '''

    def __init__(self, parent=None, opts={}, factory_class=None,
                 context=None):
        '''
        @parent Parent scope. None if this is a global scope.

        @opts   Attributes of the scope's owner (a Primitive).
                These are used so that the scope is aware of what namespace
                it's in, for instance, or what file to write the scope contents
                to. we don't have to keep bidirectional references between the
                scope and its owner.

        @factory_class -- mandatory for topmost scope, otherwise must be None
                The factory class that we instantiate to create primitive
                factories for each LogicalScope.

        @context -- optional for topmost scope, otherwise must be None
                The context that we write to. If the topmost scope isn't given
                a context, we keep track of the primitives internally, and can
                write out the entire scope using self.commit().

        '''
        # definitely should be strong pointer because scopes should be
        # destructed in the order of stack unwinding.
        # A scope's child owns the parent scope :O
        self._parent = parent
        # Also save a reference to the topmost scope
        # either our parent's parent, or ourselves if we've got no parent
        self._topmost = parent is not None and parent.topmost or self

        if parent is not None:
            if not isinstance(parent, LogicalScope):
                raise TypeError("A LogicalScope's parent must be another "
                                "LogicalScope or None")
            assert context is None, \
                'Only the topmost scope may keep a context reference'
        # this is the topmost scope
        else:
            # we're live if we have been instantiated with a context.
            # This flag cannot change, it's set in stone
            self._live = context is not None
            if self._live:
                self._set_context(context)

        # make the options more easily accessible
        self._opts = Values(opts)
        if factory_class is not None:
            assert parent is None, \
              'Only the topmost scope holds the reference to the factory class'
            self.primitive_factory_class = factory_class
        else:
            assert parent is not None, \
              "Cannot provide factory_class argument to non-topmost scopes"

        if self.is_live is False:  # property of topmost scope
            # allocate an in-memory store for children of this scope
            self._children = []

        # create a factory object, used to add Primitives to this scope
        self._factory = self._factory_class(weakref.ref(self))
        self._acquired = 0

    # SERIALIZATION METHODS
    # =====================

    def __getstate__(self):
        'Return only non-transitive data'
        persistent = ('_parent', '_children', '_topmost', '_opts')
        return dict((attr, getattr(self, attr)) for attr in persistent \
                    if hasattr(self, attr))

    def __setstate__(self, state):
        self.__dict__.update(state)
        if self._parent is None:
            self._live = False
        self._acquired = 0

    # RAII acquire/release METHODS
    # ============================

    def acquire(self):
        """Call the enter_scope callback set by the owner Primitive, or
        otherwise enter_scope on the context if this is the topmost scope"""
        assert self.is_live == True, "Can't acquire a non-live scope"
        if self._acquired > 0:
            raise Exception('Can not reacquire scope')
        changes = {}
        if self.parent is not None:
            returned = self.enter_scope_callback(self._global_context,
                                                 self)
            if returned:
                changes = returned
        self._global_context.enter_scope(self, **changes)
        self._acquired = 1
        return self

    def __del__(self):
        '''Raise an error if a LogicalScope goes out of scope before it was
        released'''
        if not self.is_live:
            return
        assert self._acquired == 2, ("LogicalScope: {0} with parent {1} "
        "went out of scope before being released. Did you forget to call"
        " .release() on it?".format(str(self.opts.__dict__),
                                    str(self.parent.opts.__dict__)))

    def release(self):
        """Call the exit_scope callback set by the owner Primitive, or
        otherwise exit_scope on the context if this is the topmost scope"""
        assert self.is_live == True, "Can't release a non-live scope"
        if self._acquired == 0:
            raise Exception('Must first acquire scope, then release')
        elif self._acquired == 2:
            # Do nothing if already released
            return
        changes = {}
        if self.parent is not None:
            returned = self.exit_scope_callback(self._global_context,
                                                self)
            if returned:
                changes = returned
        self._global_context.exit_scope(self, **changes)
        self._acquired = 2

    def empty(self):
        'Shorthand for creating an empty scope'
        self.acquire().release()

    def _write(self, primitive):
        'Write a primitive to the context. Acquire if necessary'
        assert self.is_live == True, "Can't write unless we're live"
        if self._acquired == 0:
            # Alternatively throw an exception if it's not acquired
            # raise Exception('Cannot write to scope before acquiring it')
            self.acquire()
        if self._global_context is None:
            raise RuntimeError("Cannot write a primitive to a scope without "
                "a context associated with that scope's hierarchy. Did you "
                "forget to call ")
        primitive._write(self._global_context)

    def __enter__(self):
        if self.is_live:
            self.acquire()
        return self

    def __exit__(self, *args):
        if self.is_live:
            self.release()

    @property
    def opts(self):
        return self._opts

    @property
    def parent(self):
        return self._parent

    @property
    def topmost(self):
        return self._topmost

    @property
    def _factory_class(self):
        'The primitive factory class is only referenced in the topmost scope'
        return getattr(self.topmost, 'primitive_factory_class', None)

    @property
    def _global_context(self):
        'The context is only referenced in the topmost scope'
        return getattr(self.topmost, '_context', None)

    def _set_context(self, context):
        ''' Set the context, which the entire hierarchy uses.
        Must be called on the topmost scope.
        This should only be used when the scope hierarchy is live and while
        committing (i.e. the topmost scope shouldn't have any scope associated
        when you serialize it).'''
        assert context is None or isinstance(context, OutputContext)
        assert self._parent is None, 'Scope must be topmost.'
        self._context = context

    @property
    def is_live(self):
        "We're 'live' if we were initially instantiated with a context"
        return self.topmost._live

    # Awesome shorthands, these two :)
    # You just call the scope and it automatically calls the factory to create
    # a primitive, writes it, then returns it
    def __call__(self, *args, **kwargs):
        primitive = self._factory(*args, **kwargs)
        self.add(primitive)
        return primitive

    # Shorthand for attributes of the PrimitiveFactory
    def __getattr__(self, attr):
        # only redirect to public methods of the PrimitiveFactory
        if attr.startswith('_'):
            raise AttributeError("LogicalScope has no attribute " + attr)
        func = getattr(self._factory, attr)
        # wrap it

        def wrapper(*args, **kwargs):
            #print 'Wrapper for .{0}: '.format(attr), args, kwargs
            #print '  to call:', func
            p = func(*args, **kwargs)
            self.add(p)
            return p
        return wrapper

    def add(self, primitive):
        '''Add a primitive to the children of this LogicalScope.
        Depending on the value of self.is_life, either saves this primitive to
        the scope's internal storage, or writes it to the context directly.'''
        if self.is_live:
            self._write(primitive)
        else:
            self._children.append(primitive)

    def recursive_dump(self):
        'Recursively print the internal representation of this scope.'
        raise NotImplementedError

    def commit(self, context):
        '''Recursively writes this scope to the context provided.
        This must be a topmost scope (parent is None).'''
        if self.parent is not None:
            raise RuntimeError("Attempted to commit a non-topmost scope.")
        if self.is_live:
            raise RuntimeError("Cannot commit a live scope. Pass None as the "
                               "context when instantiating the global scope "
                               "to make it non-live.")
        self._set_context(context)
        # go "live" so that acquire, write, release work, and then switch back
        self.topmost._live = True
        self._commit()
        self.topmost._live = False
        self._set_context(None)

    def _commit(self):
        self.acquire()
        for child in self._children:
            self._write(child)
            # and write its scope if it has one
            if child._scope is not None:
                child._scope._commit()
        self.release()
        # We can commit a non-live scope multiple times. Reset the _acquired
        # flag to its initial state so that we can acquire() again
        self._acquired = 0


class Scope:
    '''
    Abstract class that can be acquired to open and automatically close a
    language scope while writing to an Output
    '''

    def __init__(self, out):
        'out - an Output that we write to'
        assert isinstance(out, Output)
        self._out = out

    ### Extensible methods

    def acquire(self):
        raise NotImplementedError

    def release(self):
        raise NotImplementedError


def create_scope_factory(scope_type, output):
    '''
    Returns a factory that instantiates scope_type objects with the
    output and other given parameters
    '''

    assert issubclass(scope_type, Scope)
    assert isinstance(output, Output)

    return functools.partial(scope_type, output)


# ---------------------------------------------------------------
# Helper functions
# ---------------------------------------------------------------

def get_global_scope(primitive_factory_class, context=None):
    return LogicalScope(factory_class=primitive_factory_class, context=context)

# ---------------------------------------------------------------
# Output Context
# ---------------------------------------------------------------

class OutputContext:
    '''
    An abstract class that structured code (Primitives inside LogicalScopes)
    can be written to.
    Provides virtual callbacks for enter_scope, exit_scope and write_primitive.
    '''

    def enter_scope(self, scope, **kwargs):
        self._enter_scope_handler(scope, **kwargs)

    def exit_scope(self, scope, **kwargs):
        self._exit_scope_handler(scope, **kwargs)

    def _enter_scope_handler(self, scope, **kwargs):
        raise NotImplementedError

    def _exit_scope_handler(self, scope, **kwargs):
        raise NotImplementedError

    def write_primitive(self, what):
        'Pass a primitive to the context to be written'
        raise NotImplementedError
