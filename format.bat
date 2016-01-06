@echo off

for /R src\ %%x in (*.cpp *.h) do (
	clang-format -style=file -i "%%x"
)