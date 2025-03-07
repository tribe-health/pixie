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

import { gql, useQuery } from '@apollo/client';
import { Button } from '@mui/material';
import { Theme } from '@mui/material/styles';
import { createStyles, makeStyles } from '@mui/styles';
import * as QueryString from 'query-string';
import { useLocation } from 'react-router';
import {
  Redirect, Link,
} from 'react-router-dom';

import { AuthMessageBox } from 'app/components';

import { BasePage } from './base';

const useStyles = makeStyles((theme: Theme) => createStyles({
  ctaGutter: {
    marginTop: theme.spacing(3),
    paddingTop: theme.spacing(3),
    borderTop: `1px solid ${theme.palette.foreground.grey1}`,
    width: '80%',
  },
}));
// eslint-disable-next-line react-memo/require-memo
export const InvitePage: React.FC = () => {
  const parsed = QueryString.parse(window.location.search);
  let inviteToken = '';
  if (parsed.invite_token && typeof parsed.invite_token === 'string') {
    inviteToken = parsed.invite_token;
  }

  const { data, loading } = useQuery<{ verifyInviteToken: boolean }>(
    gql`
      query VerifyInviteToken($inviteToken: String!){
        verifyInviteToken(inviteToken: $inviteToken)
      }
    `,
    {
      variables: { inviteToken },
      context: { connType: 'unauthenticated' },
      fetchPolicy: 'network-only',
    });

  const validToken = data?.verifyInviteToken;

  const classes = useStyles();
  const location = useLocation();
  const redirect = React.useMemo(() => ({ pathname: '/auth/signup', search: location.search }), [location]);
  const cta = React.useMemo(() => (
    <div className={classes.ctaGutter}>
      <Link to={'/auth/signup'} component={Button}>
        Go To Signup
      </Link>
    </div>
  ), [classes.ctaGutter]);
  if (loading) {
    return (
      <BasePage>
      </BasePage>
    );
  }
  if (!validToken) {
    return (
      <BasePage>
        <AuthMessageBox
          error='recoverable'
          title={'Invalid Invite link'}
          message={'Invite link is expired or improperly formatted.'}
          errorDetails={'Ask for a new invite from your org admin.'}
          cta={cta}
        />
      </BasePage>
    );
  }
  return (<Redirect
    from={'/*'}
    exact={false}
    to={redirect}
  />);
};
InvitePage.displayName = 'InvitePage';
