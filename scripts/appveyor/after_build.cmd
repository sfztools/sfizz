@echo off

iscc.exe /dARCH=%platform% innosetup.iss
move *.exe ../
