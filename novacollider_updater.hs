#!/usr/bin/runghc

{-# LANGUAGE OverloadedStrings #-}
{-# LANGUAGE ExtendedDefaultRules #-}
{-# OPTIONS_GHC -fno-warn-type-defaults #-}

import Shelly
import Prelude hiding (FilePath)
import Data.Text.Lazy as LT
import Data.Monoid
import Prelude

--default (LT.Text)

branches = [ "novacollider/cmake-defaults",
             "novacollider/dark-palette",
             "novacollider/ide-dark-colorscheme",
             "novacollider/intrusive_muladds",
             "novacollider/supernova-async-osc",
             "novacollider/supernova-minor-fixes",
             -- "novacollider/yamlcpp-0.5",
             "novacollider/ide-cleanup",
             "novacollider/ide-hacks",
             "novacollider/ide-concurrency",
             "novacollider/dll",
             "novacollider/alignment-cleanups",
             "fixes/alsa-midi-fixes",
             "fixes/freqscope-resolution",
             "topic/supernova-relaxed-c_set",
             "novacollider/cmake-modernisation",
             "novacollider/oscillator-optimizations",
             -- "novacollider/dumpparsenode_cleanup"
             "novacollider/sleep-in-helper-threads",
             "novacollider/updater"
            ]

fetch_origin = run_ "git" ["fetch", "origin"]

rebase_branch branch base =
  run_ command args
  where
    command = "git"
    args    = [ "rebase", base, branch ]

rebase_on_master branch = rebase_branch branch "origin/master"

rebase_all_branches = do
  mapM_ rebase_on_master branches

push_rebased_branch branch =
  run_ "git" ["push", "github-tim", "-f", branch]

push_all_rebased_branches = do
  mapM_ push_rebased_branch branches


merge_branch branch = do
  run_ "git" ["merge", branch]

synthesize_novacollider = do
  run_ "git" ["checkout", "-b", "novacollider/next_tip", "origin/master"]

  mapM_ merge_branch branches
  run_ "git" ["checkout", "novacollider/tip"]
  run_ "git" ["reset", "--hard", "novacollider/next_tip"]
  run_ "git" ["branch", "-D", "novacollider/next_tip"]
  push_rebased_branch "novacollider/tip"

main = shelly $ verbosely $ do
         fetch_origin
         rebase_all_branches
         push_all_rebased_branches
         synthesize_novacollider
