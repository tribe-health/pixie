/*
 * Copyright 2018- The Pixie Authors.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

import * as React from 'react';

import { Theme } from '@mui/material/styles';
import { createStyles, withStyles, WithStyles } from '@mui/styles';
import { Link } from 'react-router-dom';

import { SemanticType } from 'app/types/generated/vizierapi_pb';
import { Arguments } from 'app/utils/args-utils';

import {
  deepLinkURLFromScript, deepLinkURLFromSemanticType, EmbedState,
} from './live-view-params';

const styles = ({ palette }: Theme) => createStyles({
  root: {
    '&:hover': {
      color: palette.secondary.main,
      textDecoration: 'underline',
      opacity: 0.7,
    },
    textDecoration: 'none',
    color: palette.text.primary,
    backgroundColor: 'transparent',
  },
});

/**
 * DeepLink is used when we are creating a deep link to another script for an input
 * value based on the semantic type of the column the value belongs to. For example,
 * the value `pl/pl-nats-0` would deep link to `px/pod` when the semantic type is
 * equal to ST_POD_NAME.
 */
export interface DeepLinkProps extends WithStyles<typeof styles>{
  // replace entity with `value`.
  value: string;
  semanticType: SemanticType;
  clusterName: string;
  embedState: EmbedState;
  propagatedParams?: Arguments;
}

const DeepLinkPlain = React.memo<DeepLinkProps>(({
  value, semanticType, clusterName, classes, embedState, propagatedParams,
}) => {
  const path = deepLinkURLFromSemanticType(semanticType, value, clusterName, embedState,
    propagatedParams);
  if (embedState?.widget) {
    return <>{value}</>;
  }
  return (
    <Link to={path} className={classes.root}>{value}</Link>
  );
});
DeepLinkPlain.displayName = 'DeepLinkPlain';

export const DeepLink = withStyles(styles)(DeepLinkPlain);

// ScriptReference is used when we are creating a deep link from a script name.
export interface ScriptReferenceProps extends WithStyles<typeof styles>{
  label: string;
  script: string;
  clusterName: string;
  embedState: EmbedState;
  args: Arguments;
}

const ScriptReferencePlain = React.memo<ScriptReferenceProps>(({
  label, script, args, embedState, clusterName, classes,
}) => {
  const path = deepLinkURLFromScript(script, clusterName, embedState, args);

  if (embedState.widget) {
    return <>{label}</>;
  }
  return (
    <Link to={path} className={classes.root}>{label}</Link>
  );
});
ScriptReferencePlain.displayName = 'ScriptReferencePlain';

export const ScriptReference = withStyles(styles)(ScriptReferencePlain);
