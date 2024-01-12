import {ClockApi} from './api.js';
import {handleClearConfig, handleConfigChange} from "./configExplorer.js";
import {createReleaseList} from "./github.js";
import {generateCurrentOtaPartitionHtml, generateOtherOtaPartitionHtml} from "./ui.js";

let lastMessage = null;

const handleOnline = () => {
    const status = document.getElementById('online');
    status.innerText = 'Online';
    status.classList.remove('bg-danger');
    status.classList.add('bg-success');
};

const handleOffline = () => {
    const status = document.getElementById('online');
    status.innerText = 'Offline';
    status.classList.remove('bg-success');
    status.classList.add('bg-danger');
};

const handleUpdateLastMessage = () => {
    if (!lastMessage) {
        return;
    }

    // check if last message is older than 5 seconds
    if (Date.now() - lastMessage > 5000) {
        handleOffline();
    } else {
        handleOnline();
    }
};

window.addEventListener('load', async () => {
    setInterval(handleUpdateLastMessage, 250);

    const isDev = window.location.hostname === 'localhost';
    window.clockApi = new ClockApi(isDev);

    window.clockApi.on('onStatusChange', response => {
        console.log('onStatusChange', response);
        const { status } = response;

        document.title = status.name;
        document.getElementById('deviceName').innerText = status.name;

        lastMessage = Date.now();
    });

    window.clockApi.on('onOtaStatusChange', ota => {
        console.log('onOtaStatusChange', ota);

        lastMessage = Date.now();
        handleUpdateLastMessage();

        const progressBar = document.getElementById('otaProgress');
        const otaMessage = document.getElementById('otaMessage');
        const status = document.getElementById('otaStatus');
        progressBar.style.width = `${Math.round(ota.percentage)}%`;
        progressBar.innerText = `${Math.round(ota.percentage)}%`;

        if (progressBar.style.width === '100%') {
            progressBar.classList.remove('progress-bar-striped', 'progress-bar-animated');
            progressBar.classList.add('bg-success');
        } else {
            progressBar.classList.add('progress-bar-striped', 'progress-bar-animated');
            progressBar.classList.remove('bg-success');
        }

        otaMessage.innerText = `Ota Message: ${ota.otaMessage || '-'}`;
        status.innerText = ota.isInProgress ? `Ota Status: In progress (${ota.progress}/${ota.totalSize ?? 0})` : 'Ota Status: Idle';

        const currentOtaPartition = document.getElementById('currentOtaPartition');
        currentOtaPartition.innerHTML = generateCurrentOtaPartitionHtml(ota.currentApp);

        const otherOtaPartition = document.getElementById('otherOtaPartition');
        otherOtaPartition.innerHTML = generateOtherOtaPartitionHtml(ota.otherApp);
    });

    window.clockApi.on('onConfigChange', config => {
        console.log('onConfigChange', config);

        lastMessage = Date.now();
        handleUpdateLastMessage();

        handleConfigChange(config);
    });

    window.clockApi.on('onTasksChange', tasks => {
        console.log('onTasksChange', tasks);

        lastMessage = Date.now();
        handleUpdateLastMessage();
    });

    window.clockApi.on('onOffline', () => {
        handleClearConfig();

        lastMessage = null;
        handleUpdateLastMessage();
    });

    document.getElementById('switchOta').addEventListener('click', async () => {
        const result = await window.clockApi.switchOta();
        document.getElementById('switchOtaResult').innerText = JSON.stringify(result, null, 4);
    });

    document.getElementById('reboot').addEventListener('click', async () => {
        await window.clockApi.reboot();
    });

    document.getElementById('triggerOtaForm').addEventListener('submit', async (event) => {
        event.preventDefault();

        const formData = new FormData(event.target);
        const url = formData.get('url');
        const result = await window.clockApi.triggerOta(url);
        document.getElementById('triggerOtaResult').innerText = JSON.stringify(result, null, 4);
    });

    const releaseList = await createReleaseList();

    document.getElementById('releases').appendChild(releaseList);
});


(() => {
    'use strict'

    const getStoredTheme = () => localStorage.getItem('theme');
    const setStoredTheme = theme => localStorage.setItem('theme', theme);

    const getPreferredTheme = () => {
        const storedTheme = getStoredTheme();
        if (storedTheme) {
            return storedTheme;
        }

        return window.matchMedia('(prefers-color-scheme: dark)').matches ? 'dark' : 'light';
    };

    const setTheme = theme => {
        if (theme === 'auto' && window.matchMedia('(prefers-color-scheme: dark)').matches) {
            document.documentElement.setAttribute('data-bs-theme', 'dark');
        } else {
            document.documentElement.setAttribute('data-bs-theme', theme);
        }
    };

    setTheme(getPreferredTheme());

    const showActiveTheme = (theme, focus = false) => {
        const themeSwitcher = document.querySelector('#bd-theme');

        if (!themeSwitcher) {
            return;
        }

        const themeSwitcherText = document.querySelector('#bd-theme-text');
        const btnToActive = document.querySelector(`[data-bs-theme-value="${theme}"]`);

        document.querySelectorAll('[data-bs-theme-value]').forEach(element => {
            element.classList.remove('active');
            element.setAttribute('aria-pressed', 'false');
        });

        btnToActive.classList.add('active');
        btnToActive.setAttribute('aria-pressed', 'true');
        const themeSwitcherLabel = `${themeSwitcherText.textContent} (${btnToActive.dataset.bsThemeValue})`;
        themeSwitcher.setAttribute('aria-label', themeSwitcherLabel);

        if (focus) {
            themeSwitcher.focus();
        }
    };

    window.matchMedia('(prefers-color-scheme: dark)').addEventListener('change', () => {
        const storedTheme = getStoredTheme();
        if (storedTheme !== 'light' && storedTheme !== 'dark') {
            setTheme(getPreferredTheme());
        }
    });

    window.addEventListener('DOMContentLoaded', () => {
        showActiveTheme(getPreferredTheme());

        document.querySelectorAll('[data-bs-theme-value]')
            .forEach(toggle => {
                toggle.addEventListener('click', () => {
                    const theme = toggle.getAttribute('data-bs-theme-value');
                    setStoredTheme(theme);
                    setTheme(theme);
                    showActiveTheme(theme, true);
                });
            });
    });
})();
