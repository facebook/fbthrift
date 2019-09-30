def inspect(cls):
  if not isinstance(cls, type):
    cls = type(cls)
  if hasattr(cls, '__get_reflection__'):
    return cls.__get_reflection__()
  raise TypeError('No reflection information found')


def inspectable(cls):
  if not isinstance(cls, type):
    cls = type(cls)
  return hasattr(cls, '__get_reflection__')
