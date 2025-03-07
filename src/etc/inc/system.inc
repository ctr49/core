<?php

/*
 * Copyright (C) 2016-2021 Franco Fichtner <franco@opnsense.org>
 * Copyright (C) 2004-2007 Scott Ullrich <sullrich@gmail.com>
 * Copyright (C) 2003-2004 Manuel Kasper <mk@neon1.net>
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice,
 *    this list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES,
 * INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY
 * AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
 * AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY,
 * OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

function system_powerd_configure($verbose = false)
{
    global $config;

    if (is_process_running('powerd')) {
        exec('/usr/bin/killall powerd');
    }

    if (!isset($config['system']['powerd_enable'])) {
        return;
    }

    if ($verbose) {
        echo 'Starting power daemon...';
        flush();
    }

    $ac_mode = 'hadp';
    if (!empty($config['system']['powerd_ac_mode'])) {
        $ac_mode = $config['system']['powerd_ac_mode'];
    }

    $battery_mode = 'hadp';
    if (!empty($config['system']['powerd_battery_mode'])) {
        $battery_mode = $config['system']['powerd_battery_mode'];
    }

    $normal_mode = 'hadp';
    if (!empty($config['system']['powerd_normal_mode'])) {
        $normal_mode = $config['system']['powerd_normal_mode'];
    }

    mwexecf(
        '/usr/sbin/powerd -b %s -a %s -n %s',
        array($battery_mode, $ac_mode, $normal_mode)
    );

    if ($verbose) {
        echo "done.\n";
    }
}

function get_default_sysctl_value($id)
{
    global $config;

    $sysctls = array(
        'hw.ibrs_disable' => '0',
        'hw.ixl.enable_head_writeback' => '0',
        'hw.syscons.kbd_reboot' => '0',
        'hw.uart.console' => 'io:0x3f8,br:' . system_console_speed(), /* XXX support comconsole_port if needed */
        'kern.ipc.maxsockbuf' => '4262144',
        'kern.randompid' => '1',
        'net.enc.in.ipsec_bpf_mask' => '2', /* after processing */
        'net.enc.in.ipsec_filter_mask' => '2', /* after processing */
        'net.enc.out.ipsec_bpf_mask' => '1', /* before processing */
        'net.enc.out.ipsec_filter_mask' => '1', /* before processing */
        'net.inet.icmp.drop_redirect' => '0',
        'net.inet.icmp.icmplim' => '0',
        'net.inet.icmp.log_redirect' => '0',
        'net.inet.icmp.reply_from_interface' => '1',
        'net.inet.ip.accept_sourceroute' => '0',
        'net.inet.ip.forwarding' => '1',
        'net.inet.ip.intr_queue_maxlen' => '1000',
        'net.inet.ip.portrange.first' => '1024',
        'net.inet.ip.random_id' => '1',
        'net.inet.ip.redirect' => '0',
        'net.inet.ip.sourceroute' => '0',
        'net.inet.tcp.blackhole' => '2',
        'net.inet.tcp.delayed_ack' => '0',
        'net.inet.tcp.drop_synfin' => '1',
        'net.inet.tcp.log_debug' => '0',
        'net.inet.tcp.recvspace' => '65228',
        'net.inet.tcp.sendspace' => '65228',
        'net.inet.tcp.syncookies' => '1',
        'net.inet.tcp.tso' => '1',
        'net.inet.udp.blackhole' => '1',
        'net.inet.udp.checksum' => 1,
        'net.inet.udp.maxdgram' => '57344',
        'net.inet6.ip6.forwarding' => '1',
        'net.inet6.ip6.prefer_tempaddr' => '0',
        'net.inet6.ip6.redirect' => '0',
        'net.inet6.ip6.use_tempaddr' => '0',
        'net.link.bridge.pfil_bridge' => '0',
        'net.link.bridge.pfil_local_phys' => '0',
        'net.link.bridge.pfil_member' => '1',
        'net.link.bridge.pfil_onlyip' => '0',
        'net.link.tap.user_open' => '1',
        'net.link.vlan.mtag_pcp' => '1',
        'net.local.dgram.maxdgram' => '8192',
        'security.bsd.see_other_gids' => '0',
        'security.bsd.see_other_uids' => '0',
        'vfs.read_max' => '32',
        'vm.pmap.pti' => '1',
    );

    if (isset($sysctls[$id])) {
        return $sysctls[$id];
    }

    return null;
}

function system_sysctl_get()
{
    /* list of unconditional sysctls not necessarily present in config */
    $sysctls = array(
        'hw.ixl.enable_head_writeback' => 'default',
        'net.enc.in.ipsec_bpf_mask' => 'default',
        'net.enc.in.ipsec_filter_mask' => 'default',
        'net.enc.out.ipsec_bpf_mask' => 'default',
        'net.enc.out.ipsec_filter_mask' => 'default',
        'net.inet.icmp.reply_from_interface' => 'default',
        'net.inet.ip.forwarding' => 'default',
        'net.inet6.ip6.forwarding' => 'default',
        'net.link.vlan.mtag_pcp' => 'default',
        'net.local.dgram.maxdgram' => 'default',
    );

    foreach (config_read_array('sysctl', 'item') as $tunable) {
        $sysctls[$tunable['tunable']] = $tunable['value'];
    }

    foreach ($sysctls as $key => &$value) {
        if ($value != 'default') {
            continue;
        }

        $value = get_default_sysctl_value($key);

        if ($value === null) {
            log_error('warning: ignoring missing default tunable request: ' . $key);
            unset($sysctls[$key]);
        }
    }

    ksort($sysctls);

    return $sysctls;
}

function system_resolvconf_generate($verbose = false)
{
    global $config;

    if ($verbose) {
        echo 'Generating /etc/resolv.conf...';
        flush();
    }

    $syscfg = config_read_array('system');
    $resolvconf = '';
    $search = [];

    if (!empty($syscfg['domain'])) {
        $resolvconf = "domain {$syscfg['domain']}\n";
        $search[] = $syscfg['domain'];
    }

    if (!isset($syscfg['dnslocalhost']) && (isset($config['dnsmasq']['enable']) || isset($config['unbound']['enable']))) {
        $resolvconf .= "nameserver 127.0.0.1\n";
    }

    if (isset($syscfg['dnsallowoverride'])) {
        $search = array_merge($search, get_searchdomains());
        foreach (get_nameservers() as $nameserver) {
            $resolvconf .= "nameserver $nameserver\n";
        }
    }

    if (isset($syscfg['dnsserver'][0])) {
        foreach ($syscfg['dnsserver'] as $ns) {
            $resolvconf .= "nameserver $ns\n";
        }
    }

    if (count($search)) {
        $result = $search[0];
        /* resolv.conf search keyword limit is 6 domains, 256 characters */
        foreach (range(2, 6) as $len) {
            if (count($search) < $len) {
                break;
            }
            $temp = implode(' ', array_slice($search, 0, $len));
            if (strlen($temp) >= 256) {
                break;
            }
            $result = $temp;
        }
        $resolvconf .= "search {$result}\n";
    }

    $tempfile = tempnam('/tmp', 'resolv.conf');
    file_put_contents($tempfile, $resolvconf);
    chmod($tempfile, 0644);

    rename($tempfile, '/etc/resolv.conf');

    /* setup static routes for DNS servers. */
    $gateways = new \OPNsense\Routing\Gateways(legacy_interfaces_details());
    for ($dnscounter = 1; $dnscounter < 9; $dnscounter++) {
        /* setup static routes for dns servers */
        $dnsgw = "dns{$dnscounter}gw";
        if (isset($syscfg[$dnsgw])) {
            $gwname = $syscfg[$dnsgw];
            if (($gwname != '') && ($gwname != 'none')) {
                $gatewayip = $gateways->getAddress($gwname);
                if (is_ipaddrv4($gatewayip)) {
                    /* dns server array starts at 0 */
                    $dnscountermo = $dnscounter - 1;
                    system_host_route($syscfg['dnsserver'][$dnscountermo], $gatewayip);
                }
                if (is_ipaddrv6($gatewayip)) {
                    /* dns server array starts at 0 */
                    $dnscountermo = $dnscounter - 1;
                    system_host_route($syscfg['dnsserver'][$dnscountermo], $gatewayip);
                }
            }
        }
    }

    if ($verbose) {
        echo "done.\n";
    }
}

function get_locale_list()
{
    $locales = array();

    /* first one is the default */
    $locales['en_US'] = gettext('English');
    $locales['cs_CZ'] = gettext('Czech');
    $locales['zh_CN'] = gettext('Chinese (Simplified)');
    $locales['fr_FR'] = gettext('French');
    $locales['de_DE'] = gettext('German');
    $locales['it_IT'] = gettext('Italian');
    $locales['ja_JP'] = gettext('Japanese');
    $locales['no_NO'] = gettext('Norwegian');
    $locales['pt_BR'] = gettext('Portuguese (Brazil)');
    $locales['pt_PT'] = gettext('Portuguese (Portugal)');
    $locales['ru_RU'] = gettext('Russian');
    $locales['es_ES'] = gettext('Spanish');
    $locales['tr_TR'] = gettext('Turkish');
    $locales['vi_VN'] = gettext('Vietnamese');

    return $locales;
}

function get_country_codes()
{
    $dn_cc = array();

    $iso3166_tab = '/usr/local/opnsense/contrib/tzdata/iso3166.tab';
    if (file_exists($iso3166_tab)) {
        $dn_cc_file = file($iso3166_tab);
        foreach ($dn_cc_file as $line) {
            if (preg_match('/^([A-Z][A-Z])\t(.*)$/', $line, $matches)) {
                $dn_cc[$matches[1]] = trim($matches[2]);
            }
        }
    }
    return $dn_cc;
}

function get_zoneinfo()
{
    $zones = timezone_identifiers_list(DateTimeZone::ALL ^ DateTimeZone::UTC);

    $etcs = glob('/usr/share/zoneinfo/Etc/*');
    foreach ($etcs as $etc) {
        $zones[] = ltrim($etc, '/usr/share/zoneinfo/');
    }

    natsort($zones);

    return $zones;
}

function get_searchdomains()
{
    $master_list = array();

    $search_list = glob('/tmp/*_searchdomain*');

    if (is_array($search_list)) {
        foreach ($search_list as $fdns) {
            $contents = file($fdns, FILE_IGNORE_NEW_LINES | FILE_SKIP_EMPTY_LINES);
            if (!is_array($contents)) {
                continue;
            }
            foreach ($contents as $dns) {
                if (!empty($dns) && is_hostname($dns)) {
                    $master_list[] = $dns;
                }
            }
        }
    }

    return array_unique($master_list);
}

function get_nameservers($interface = null)
{
    global $config;
    $master_list = array();

    $dns_lists = glob('/tmp/*_nameserver*');
    $exclude_interfaces = [];

    if (!empty($interface)) {
        /* only acquire servers provided for this interface */
        $realif = get_real_interface($interface);
        $realifv6 = get_real_interface($interface, 'inet6');
        $dns_lists = [
            "/tmp/{$realif}_nameserver",
            "/tmp/{$realifv6}_nameserverv6",
        ];
    }

    if (isset($config['system']['dnsallowoverride_exclude'])) {
        foreach (explode(',', $config['system']['dnsallowoverride_exclude']) as $intf) {
            if (isset($config['interfaces'][$intf])) {
                $exclude_interfaces[] = get_real_interface($intf);
                $exclude_interfaces[] = get_real_interface($intf, 'inet6');
            }
        }
    }

    foreach ($dns_lists as $fdns) {
        $intf = explode('_', basename($fdns))[0];
        if (in_array($intf, $exclude_interfaces)) {
            continue;
        }

        $contents = @file($fdns, FILE_IGNORE_NEW_LINES | FILE_SKIP_EMPTY_LINES);
        if (!is_array($contents)) {
            continue;
        }
        foreach ($contents as $dns) {
            if (!empty($dns) && is_ipaddr($dns)) {
                $master_list[] = $dns;
            }
        }
    }

    return array_unique($master_list);
}

function system_hosts_generate($verbose = false)
{
    global $config;

    if ($verbose) {
        echo 'Generating /etc/hosts...';
        flush();
    }

    $syscfg = config_read_array('system');

    $hosts = "127.0.0.1\tlocalhost localhost.{$syscfg['domain']}\n";

    /* we pull the second host entry from a "priority" list: lan, optX or wan */
    $iflist = array_keys(get_configured_interface_with_descr());
    sort($iflist);

    if (!empty($iflist[0])) {
        $cfgip = get_interface_ip($iflist[0]);
        if (is_ipaddrv4($cfgip)) {
            $hosts .= "{$cfgip}\t{$syscfg['hostname']}.{$syscfg['domain']} {$syscfg['hostname']}\n";
        }
    }

    file_put_contents('/etc/hosts', $hosts);

    plugins_configure('hosts');

    if ($verbose) {
        echo "done.\n";
    }
}

function system_hostname_configure($verbose = false)
{
    if ($verbose) {
        echo 'Setting hostname: ';
        flush();
    }

    $syscfg = config_read_array('system');

    $hostname = "{$syscfg['hostname']}.{$syscfg['domain']}";

    mwexecf('/bin/hostname %s', $hostname);

    if ($verbose) {
        echo "{$hostname}\n";
    }
}

function system_host_route($host, $gateway, $delete = true, $add = true)
{
    if (is_ipaddrv4($gateway)) {
        $family = 'inet';
    } elseif (is_ipaddrv6($gateway)) {
        $family = 'inet6';
    } else {
        return;
    }

    if ($delete) {
        mwexecf('/sbin/route delete -host -%s %s', array($family, $host), true);
    }

    if ($add) {
        mwexecf('/sbin/route add -host -%s %s %s', array($family, $host, $gateway));
    }
}

function system_default_route($gateway, $family, $interface, $far = false)
{
    $realif = get_real_interface($interface, $family == 'inet' ? 'all' : 'inet6');

    switch ($family) {
        case 'inet':
            break;
        case 'inet6':
            if (is_linklocal($gateway)) {
                $gateway .= "%{$realif}";
            }
            break;
        default:
            log_error("ROUTING: unknown address family '{$family}'");
            return;
    }

    $tmpcmd = "/sbin/route -n get -{$family} default 2>/dev/null | /usr/bin/awk '/gateway:/ {print $2}'";
    $current = trim(exec($tmpcmd), " \n");
    if ($current == $gateway) {
        log_error("ROUTING: keeping current default gateway '{$gateway}'");
        return;
    }

    if ($family == 'inet') {
        foreach (glob('/tmp/*_defaultgw') as $to_delete) {
            log_error("ROUTING: removing {$to_delete}");
            @unlink($to_delete);
        }

        log_error("ROUTING: creating /tmp/{$realif}_defaultgw using '{$gateway}'");
        @file_put_contents("/tmp/{$realif}_defaultgw", $gateway);

        if (!$far) {
            $realif = null;
        }
    } else {
        foreach (glob('/tmp/*_defaultgwv6') as $to_delete) {
            log_error("ROUTING: removing {$to_delete}");
            @unlink($to_delete);
        }

        log_error("ROUTING: creating /tmp/{$realif}_defaultgwv6 using '{$gateway}'");
        @file_put_contents("/tmp/{$realif}_defaultgwv6", $gateway);

        /* IPv6 does not support far gateway notion */
        $realif = null;
    }

    mwexecf('/sbin/route delete -%s default', array($family), true);
    if (!empty($realif)) {
        mwexecf('/sbin/route delete -%s %s -interface %s', array($family, $gateway, $realif), true);
        mwexecf('/sbin/route add -%s %s -interface %s', array($family, $gateway, $realif));
    }
    mwexecf('/sbin/route add -%s default %s', array($family, $gateway));
}

function system_routing_configure($verbose = false, $interface = '')
{
    if ($verbose) {
        echo 'Setting up routes...';
        flush();
    }

    if (!empty($interface)) {
        log_error("ROUTING: entering configure using '${interface}'");
    } else {
        log_error("ROUTING: entering configure using defaults");
    }

    $gateways = new \OPNsense\Routing\Gateways(legacy_interfaces_details());

    foreach (['inet', 'inet6'] as $ipproto) {
        /* determine default gateway without considering monitor status */
        $gateway = $gateways->getDefaultGW(array(), $ipproto);
        $logproto = $ipproto == 'inet' ? 'IPv4' : 'IPv6';
        if ($gateway != null) {
            log_error("ROUTING: {$logproto} default gateway set to {$gateway['interface']}");
            if ((empty($interface) || $interface == $gateway['interface']) && !empty($gateway['gateway'])) {
                log_error("ROUTING: setting {$logproto} default route to {$gateway['gateway']}");
                system_default_route($gateway['gateway'], $ipproto, $gateway['interface'], isset($gateway['fargw']));
            } else {
                log_error("ROUTING: skipping {$logproto} default route");
            }
        }
    }

    system_staticroutes_configure($interface);

    if ($verbose) {
        echo "done.\n";
    }
}

function system_staticroutes_configure($interface = '')
{
    $static_routes = get_staticroutes(false);
    if (count($static_routes)) {
        $ifdetails = legacy_interfaces_details();
        $gateways_arr = (new \OPNsense\Routing\Gateways($ifdetails))->gatewaysIndexedByName(false, true);
        foreach ($static_routes as $rtent) {
            if (empty($gateways_arr[$rtent['gateway']])) {
                log_error(sprintf('Static Routes: Gateway IP could not be found for %s', $rtent['network']));
                continue;
            }
            $gateway = $gateways_arr[$rtent['gateway']];
            if (!empty($interface) && $interface != $gateway['interface']) {
                continue;
            }

            if (!is_subnet($rtent['network'])) {
                log_error(sprintf('Cannot add static route to: %s', $rtent['network']));
                continue;
            }
            $interfacegw = $gateway['if'];
            $gatewayip = $gateway['gateway'];
            $fargw = isset($gateway['fargw']) && $gateway['ipprotocol'] != 'inet6';
            $blackhole = '';

            switch ($rtent['gateway']) {
                case 'Null4':
                case 'Null6':
                    $blackhole = '-blackhole';
                    break;
                default:
                    break;
            }

            $ip = $rtent['network'];
            if (!empty($rtent['disabled'])) {
                $inet = (is_subnetv6($ip) ? "-inet6" : "-inet");
                mwexec("/sbin/route delete {$inet} " . escapeshellarg($ip), true);
            } else {
                $inet = (is_subnetv6($ip) ? "-inet6" : "-inet");
                $cmd = " {$inet} {$blackhole} " . escapeshellarg($ip) . " ";
                if (is_ipaddr($gatewayip)) {
                    mwexec("/sbin/route delete" . $cmd . escapeshellarg($gatewayip), true);
                    if ($fargw) {
                        mwexecf('/sbin/route delete %s %s -interface %s ', array($inet, $gatewayip, $interfacegw), true);
                        mwexecf('/sbin/route add %s %s -interface %s', array($inet, $gatewayip, $interfacegw), true);
                    } elseif (is_linklocal($gatewayip) && strpos($gatewayip, '%') === false) {
                        $gatewayip .= "%{$interfacegw}";
                    }
                    mwexec("/sbin/route add" . $cmd . escapeshellarg($gatewayip), true);
                } elseif (!empty($interfacegw)) {
                    mwexec("/sbin/route delete" . $cmd . "-interface " . escapeshellarg($interfacegw), true);
                    mwexec("/sbin/route add" . $cmd . "-interface " . escapeshellarg($interfacegw), true);
                }
            }
        }
    }
}

function system_syslog_start($verbose = false)
{
    global $config;

    if ($verbose) {
        echo 'Configuring system logging...';
        flush();
    }

    configd_run('template reload OPNsense/Syslog');
    mwexecf('/usr/local/opnsense/scripts/syslog/generate_certs');

    if (isvalidpid('/var/run/syslog-ng.pid')) {
        mwexecf('/usr/local/sbin/syslog-ng-ctl reload');
    } else {
        mwexecf('/usr/sbin/service syslog-ng restart');
    }

    if ($verbose) {
        echo "done.\n";
    }
}

function system_syslog_stop()
{
    mwexecf('/usr/sbin/service syslog-ng stop');
}

/*
 *     get_memory()
 *     returns an array listing the amount of
 *     memory installed in the hardware
 *     [0] net memory available for the OS (FreeBSD) after some is taken by BIOS, video or whatever - e.g. 235 MBytes
 *     [1] real (actual) memory of the system, should be the size of the RAM card/s - e.g. 256 MBytes
 */
function get_memory()
{
    $physmem = get_single_sysctl("hw.physmem");
    $realmem = get_single_sysctl("hw.realmem");
    /* convert from bytes to megabytes */
    return array(($physmem / 1048576),($realmem / 1048576));
}

function system_firmware_configure($verbose = false)
{
    global $config;

    if ($verbose) {
        echo 'Writing firmware setting...';
        flush();
    }

    /* calculate the effective ABI */
    $args = [ exec_safe('-A %s', trim(shell_exec('opnsense-version -x'))) ];

    if (!empty($config['system']['firmware']['mirror'])) {
        $args[] = exec_safe('-m %s', str_replace('/', '\/', $config['system']['firmware']['mirror']));
    }

    if (!empty($config['system']['firmware']['flavour'])) {
        $args[] = exec_safe('-n %s', str_replace('/', '\/', $config['system']['firmware']['flavour']));
    }

    /* rewrite the config via the defaults and possible arguments */
    mwexec('/usr/local/sbin/opnsense-update -sd ' . join(' ', $args));

    if ($verbose) {
        echo "done.\n";
    }
}

function system_trust_configure($verbose = false)
{
    global $config;
    if ($verbose) {
        echo 'Writing trust files...';
        flush();
    }

    $ca_root_nss = '/usr/local/share/certs/ca-root-nss.crt';
    $ca_cert_pem = '/usr/local/openssl/cert.pem';
    if (file_exists($ca_root_nss)) {
        $ca = file_get_contents($ca_root_nss);
        foreach (config_read_array('ca') as $entry) {
            if (!empty($entry['crt'])) {
                // Split and cleans ca certificates, one entry could contain multiple certs if a user imported a bundle
                // avoid expired ca's from being considered as valid alternatives.
                $certlist = str_replace("\r", '', base64_decode($entry['crt']));
                $user_cas = [""];
                foreach (explode("\n", $certlist) as $row) {
                    $user_cas[count($user_cas) - 1] .= $row . "\n";
                    if (strpos($row, '---END') > 0) {
                        $user_cas[] = "";
                    }
                }
                $ca .= "\n# {$entry['descr']}\n";
                $include_intermediates = !empty($config['system']['store_intermediate_certs']);
                foreach ($user_cas as $user_ca) {
                    if (!empty(trim($user_ca))) {
                        $certinfo = @openssl_x509_parse($user_ca);
                        $certext = !empty($certinfo['extensions']) ? $certinfo['extensions'] : [];
                        $authoritykey = "";
                        if (!empty($certext['authorityKeyIdentifier'])) {
                            $authoritykey = trim(str_replace('keyid:', '', preg_split("/\r\n|\n|\r/", $certext['authorityKeyIdentifier'])[0]));
                        }
                        $subjectkey = $certext['subjectKeyIdentifier'];
                        $is_self_signed = empty($authoritykey) || $subjectkey == $authoritykey;

                        $error_line = "";
                        if (!empty($certinfo['validTo']) && $certinfo['validTo_time_t'] < time()) {
                            $error_line = sprintf(
                                "(system local trust) refusing to import %s from %s (expired @ %s)",
                                $certinfo['name'],
                                $entry['descr'],
                                date('r', $certinfo['validFrom_time_t'])
                            );
                        } elseif (!$include_intermediates && !$is_self_signed) {
                            $error_line = sprintf(
                                "(system local trust) skip intermediate certificate %s from %s",
                                $certinfo['name'],
                                $entry['descr']
                            );
                        }
                        if (!empty($error_line)) {
                            syslog(LOG_NOTICE, $error_line);
                            $ca .= "#" .  str_replace("\n", "", $error_line);
                        } else {
                            $ca .= $user_ca;
                        }
                    }
                }
            }
        }

        file_put_contents($ca_cert_pem, $ca);
        copy($ca_cert_pem, '/usr/local/etc/ssl/cert.pem');
        @unlink('/etc/ssl/cert.pem'); /* do not clobber symlink target */
        copy($ca_cert_pem, '/etc/ssl/cert.pem');
        chmod('/etc/ssl/cert.pem', 0644);
    }

    if ($verbose) {
        echo "done.\n";
    }
}

function system_timezone_configure($verbose = false)
{
    $syscfg = config_read_array('system');

    if ($verbose) {
        echo 'Setting timezone...';
        flush();
    }

    /* extract appropriate timezone file */
    $timezone = $syscfg['timezone'];
    $timezones = get_zoneinfo();

    /* reset to default if empty or nonexistent */
    if (
        empty($timezone) || !in_array($timezone, $timezones) ||
        !file_exists(sprintf('/usr/share/zoneinfo/%s', $timezone))
    ) {
        $timezone = 'Etc/UTC';
    }

    /* apply timezone */
    if (file_exists(sprintf('/usr/share/zoneinfo/%s', $timezone))) {
        copy(sprintf('/usr/share/zoneinfo/%s', $timezone), '/etc/localtime');
    }

    if ($verbose) {
        echo "done.\n";
    }
}

function system_sysctl_configure($verbose = false)
{
    if ($verbose) {
        echo 'Setting up extended sysctls...';
        flush();
    }

    set_sysctl(system_sysctl_get());
    system_arp_wrong_if();

    if ($verbose) {
        echo "done.\n";
    }
}

function system_arp_wrong_if()
{
    global $config;

    set_sysctl(array(
        'net.link.ether.inet.log_arp_wrong_iface' => isset($config['system']['sharednet']) ? '0' : '1',
        'net.link.ether.inet.log_arp_movements' => isset($config['system']['sharednet']) ? '0' : '1',
    ));
}

function system_kernel_configure($verbose = false)
{
    global $config;

    if ($verbose) {
        echo 'Configuring kernel modules...';
        flush();
    }

    /*
     * Vital kernel modules can go missing on reboot due to
     * /boot/loader.conf not materialising.  This is still
     * an UFS problem, despite claims otherwise.  In any case,
     * load all the modules again to make sure.
     *
     * Keep in sync with /usr/local/etc/erc.loader.d/20-modules
     */
    $mods = array(
        'carp',
        'if_bridge',
        'if_enc',
        'if_gif',
        'if_gre',
        'if_lagg',
        'if_tap',
        'if_tun',
        'if_vlan',
        'pf',
        'pflog',
        'pfsync',
    );

    if (!empty($config['system']['crypto_hardware'])) {
        log_error(sprintf('Loading %s cryptographic accelerator module.', $config['system']['crypto_hardware']));
        $mods[] = $config['system']['crypto_hardware'];
    }

    if (!empty($config['system']['thermal_hardware'])) {
        log_error(sprintf('Loading %s thermal monitor module.', $config['system']['thermal_hardware']));
        $mods[] = $config['system']['thermal_hardware'];
    }

    foreach ($mods as $mod) {
        mwexecf('/sbin/kldload %s', $mod, true);
    }

    /* we now have /dev/pf, time to fix permissions for proxies */
    chgrp('/dev/pf', 'proxy');
    chmod('/dev/pf', 0660);

    if ($verbose) {
        echo "done.\n";
    }
}

function system_devd_configure($verbose = false)
{
    if ($verbose) {
        echo 'Starting device manager...';
        flush();
    }

    exec('/sbin/devd');
    /* historic sleep */
    sleep(1);

    if ($verbose) {
        echo "done.\n";
    }
}

function system_cron_configure($verbose = false)
{
    function generate_cron_job($command, $minute = '0', $hour = '*', $monthday = '*', $month = '*', $weekday = '*')
    {
        $cron_item = array();

        $cron_item['minute'] = $minute;
        $cron_item['hour'] = $hour;
        $cron_item['mday'] = $monthday;
        $cron_item['month'] = $month;
        $cron_item['wday'] = $weekday;
        $cron_item['command'] = $command;

        return $cron_item;
    }

    $autocron = array();

    if ($verbose) {
        echo 'Configuring CRON...';
        flush();
    }

    foreach (plugins_cron() as $cron_plugin) {
        /*
         * We are stuffing jobs inside 'autocron' to be able to
         * deprecate this at a later time.  Ideally all of the
         * services should use a single cron-model, which this is
         * not.  At least this plugin function helps us to divide
         * and conquer the code bits...  :)
         */
        if (!empty($cron_plugin['autocron'])) {
            $autocron[] = call_user_func_array('generate_cron_job', $cron_plugin['autocron']);
        }
    }

    $crontab_contents = "# DO NOT EDIT THIS FILE -- OPNsense auto-generated file\n";
    $crontab_contents .= "#\n";
    $crontab_contents .= "# User-defined crontab files can be loaded via /etc/cron.d\n";
    $crontab_contents .= "# or /usr/local/etc/cron.d and follow the same format as\n";
    $crontab_contents .= "# /etc/crontab, see the crontab(5) manual page.\n";
    $crontab_contents .= "SHELL=/bin/sh\n";
    $crontab_contents .= "PATH=/etc:/bin:/sbin:/usr/bin:/usr/sbin:/usr/local/bin:/usr/local/sbin\n";
    $crontab_contents .= "REQUESTS_CA_BUNDLE=/etc/ssl/cert.pem\n";
    $crontab_contents .= "#minute\thour\tmday\tmonth\twday\tcommand\n";

    foreach ($autocron as $item) {
        $crontab_contents .= "{$item['minute']}\t";
        $crontab_contents .= "{$item['hour']}\t";
        $crontab_contents .= "{$item['mday']}\t";
        $crontab_contents .= "{$item['month']}\t";
        $crontab_contents .= "{$item['wday']}\t";
        $crontab_contents .= "({$item['command']}) > /dev/null\n";
    }

    file_put_contents('/var/cron/tabs/root', $crontab_contents);

    configd_run('template reload OPNsense/Cron');

    mwexec('/etc/rc.d/cron restart');

    if ($verbose) {
        echo "done.\n";
    }
}

function system_console_mutable()
{
    /* this function name is a pun :) */

    global $config;

    return isset($config['system']['primaryconsole']) &&
        ($config['system']['primaryconsole'] == 'serial' ||
        $config['system']['primaryconsole'] == 'null');
}

function system_console_mute()
{
    if (system_console_mutable()) {
        exec('/sbin/conscontrol mute on');
    }
}

function system_console_unmute()
{
    if (system_console_mutable()) {
        exec('/sbin/conscontrol mute off');
    }
}

function system_console_types()
{
    return array(
        /* sorted by usage */
        'video' => array('value' => 'vidconsole', 'name' => gettext('VGA Console')),
        'serial' => array('value' => 'comconsole', 'name' => gettext('Serial Console')),
        'efi' => array('value' => 'efi', 'name' => gettext('EFI Console')),
        'null' => array('value' => 'nullconsole', 'name' => gettext('Mute Console')),
    );
}

function system_console_speed()
{
    global $config;

    $speed = 115200;

    if (!empty($config['system']['serialspeed']) && is_numeric($config['system']['serialspeed'])) {
        $speed = $config['system']['serialspeed'];
    }

    return $speed;
}

function system_login_configure($verbose = false)
{
    global $config;

    if ($verbose) {
        echo 'Configuring login behaviour...';
        flush();
    }

    /* depends on user account locking */
    local_sync_accounts();

    configd_run('template reload OPNsense/Auth');

    $serialspeed = system_console_speed();

    $new_boot_config = array();
    $new_boot_config['comconsole_speed'] = null;
    $new_boot_config['boot_multicons'] = null;
    $new_boot_config['boot_serial'] = null;
    $new_boot_config['kern.vty'] = null;
    $new_boot_config['console'] = null;

    $console_types = system_console_types();
    $console_selection = array();

    foreach (array('primaryconsole', 'secondaryconsole') as $console_order) {
        if (!empty($config['system'][$console_order]) && isset($console_types[$config['system'][$console_order]])) {
            $console_selection[] = $console_types[$config['system'][$console_order]]['value'];
        }
    }

    $console_selection = array_unique($console_selection);

    $output_enabled = count($console_selection) != 1 || !in_array('nullconsole', $console_selection);
    $virtual_enabled = !count($console_selection) || in_array('vidconsole', $console_selection) ||
        in_array('efi', $console_selection);
    $serial_enabled = in_array('comconsole', $console_selection);

    if (count($console_selection)) {
        $new_boot_config['console'] = '"' . implode(',', $console_selection) . '"';
        if (count($console_selection) >= 2) {
            $new_boot_config['boot_multicons'] = '"YES"';
        }
    }

    if ($serial_enabled) {
        $serial_options = ["-S{$serialspeed}"];
        if ($console_selection[0] == 'comconsole') {
            $serial_options[] = '-h';
        }
        if (in_array('vidconsole', $console_selection)) {
            $serial_options[] = '-D';
        }
        @file_put_contents('/boot.config', join(' ', $serial_options) . "\n");
        $new_boot_config['comconsole_speed'] = '"' . $serialspeed . '"';
        $new_boot_config['boot_serial'] = '"YES"';
    } elseif (!$output_enabled) {
        @file_put_contents('/boot.config', "-q -m\n");
    } else {
        @unlink('/boot.config');
    }

    if (empty($config['system']['usevirtualterminal'])) {
        $new_boot_config['kern.vty'] = '"sc"';
    }

    /* reload static values from rc.loader.d */
    mwexecf('/usr/local/etc/rc.loader');

    /* copy settings already there */
    $new_loader_conf = @file_get_contents('/boot/loader.conf');

    $new_loader_conf .= "# dynamically generated console settings follow\n";
    foreach ($new_boot_config as $param => $value) {
        if (!empty($value)) {
            $new_loader_conf .= "{$param}={$value}\n";
        } else {
            $new_loader_conf .= "#${param}\n";
        }
    }
    $new_loader_conf .= "\n";

    $new_loader_conf .= "# dynamically generated tunables settings follow\n";
    foreach (system_sysctl_get() as $param => $value) {
        $new_loader_conf .= "{$param}=\"{$value}\"\n";
    }

    /* write merged file back to target location */
    @file_put_contents('/boot/loader.conf', $new_loader_conf);

    /* setup /etc/ttys */
    $etc_ttys_lines = explode("\n", file_get_contents('/etc/ttys'));
    $fd = fopen('/etc/ttys', 'w');

    $on_off_secure_u = $serial_enabled ? (isset($config['system']['serialusb']) ? 'on' : 'onifconsole') . ' secure' : 'off secure';
    $on_off_secure_v = $virtual_enabled ? 'onifexists secure' : 'off secure';
    if (isset($config['system']['disableconsolemenu'])) {
        $console_type = 'Pc';
        $serial_type = '3wire.' . $serialspeed;
    } else {
        $console_type = 'al.Pc';
        $serial_type = 'al.3wire.' . $serialspeed;
    }

    foreach ($etc_ttys_lines as $tty) {
        /* virtual terminals */
        foreach (array('ttyv0', 'ttyv1', 'ttyv2', 'ttyv3', 'ttyv4', 'ttyv5', 'ttyv6', 'ttyv7') as $virtualport) {
            if (strpos($tty, $virtualport) === 0) {
                fwrite($fd, "{$virtualport}\t\"/usr/libexec/getty {$console_type}\"\t\txterm\t${on_off_secure_v}\n");
                continue 2;
            }
        }
        /* serial terminals */
        foreach (array('tty%s0', 'tty%s1', 'tty%s2', 'tty%s3') as $serialport) {
            $serialport = sprintf($serialport, isset($config['system']['serialusb']) ? 'U' : 'u');
            if (stripos($tty, $serialport) === 0) {
                fwrite($fd, "{$serialport}\t\"/usr/libexec/getty {$serial_type}\"\tvt100\t{$on_off_secure_u}\n");
                continue 2;
            }
        }

        if (!empty($tty) && strpos($tty, 'xc0') === false) {
            /* all other lines stay the same */
            fwrite($fd, $tty . "\n");
        }
    }

    if (file_exists('/dev/xc0')) {
        fwrite($fd, "xc0\t\"/usr/libexec/getty {$console_type}\"\t\txterm\t${on_off_secure_v}\n");
    }

    fclose($fd);

    if ($verbose) {
        echo "done.\n";
    }

    /* force init(8) to reload /etc/ttys */
    exec('/bin/kill -HUP 1');
}

function reset_factory_defaults($sync = true)
{
    mwexec('/bin/rm -fr /conf/* /var/log/* /root/.history');
    disable_security_checks();

    mwexec('/usr/local/sbin/opnsense-beep stop');

    /* as we go through a special case directly shut down */
    $shutdown_cmd = '/sbin/shutdown -op now';
    if ($sync) {
        mwexec($shutdown_cmd);
    } else {
        mwexec_bg($shutdown_cmd);
    }
}
