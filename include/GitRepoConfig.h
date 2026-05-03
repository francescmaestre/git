#pragma once

#include <QString>

struct GitRepoConfig
{
   QString workingDirectory;
   QString gitDirectory;
   QString gitLocation;

   explicit GitRepoConfig(QString workingDir = {}, QString gitLoc = {})
       : workingDirectory(std::move(workingDir))
       , gitDirectory(workingDirectory + "/.git")
       , gitLocation(std::move(gitLoc))
   {
   }
};
