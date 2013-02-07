#!/usr/bin/python
# Filename : persion.py

class Person(object):
	count = 0 ;
	def __init__(self, name):
		print "__init__", name;

	def __del__(self):
		print "__del__";

	def add(self, cnt):
		self.cnt = cnt;
		Person.count += 1;

person = Person("yhz");
person.add(1);
print Person.count;
person.add(2);
print Person.count;
