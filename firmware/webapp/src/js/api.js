/*
 * GET  /api/v1/status
 * GET  /api/v1/config
 * GET  /api/v1/set
 * POST /api/v1/set
 * GET  /api/v1/leds
 * GET  /api/v1/tasks
 * GET  /api/v1/triggerOta (?url=)
 * GET  /api/v1/ota
 * GET  /api/v1/reboot
 */

const delay = ms => new Promise(resolve => setTimeout(resolve, ms));

export class ClockApi {
    onStatusChange;
    onConfigChange;
    onLedsChange;
    onTasksChange;
    onOtaStatusChange;
    onOffline;
    fetchInterval;

    constructor(isDev) {
        this.isDev = isDev;

        const urlParams = new URLSearchParams(window.location.search);
        const host = urlParams.get('host');

        this.apiBase = this.isDev ? `http://${host ?? '192.168.12.230'}/api/v1` : '/api/v1';

        // state
        this.status = null;
        this.config = null;
        this.leds = null;
        this.tasks = null;
        this.otastatus = null;

        // interval
        this.fetchInterval = null;

        // callbacks
        this.onStatusChange = null;
        this.onConfigChange = null;
        this.onLedsChange = null;
        this.onTasksChange = null;
        this.onOtaStatusChange = null;
        this.onOffline = null;

        this.fetching = false;

        this.init();
    }

    on(event, callback) {
        switch (event) {
            case 'onStatusChange':
            case 'onConfigChange':
            case 'onLedsChange':
            case 'onTasksChange':
            case 'onOtaStatusChange':
            case 'onOffline':
                this[`${event}`] = callback;
                break;
            default:
                throw new Error(`Unknown event: ${event}`);
        }
    }

    off(event) {
        switch (event) {
            case 'onStatusChange':
            case 'onConfigChange':
            case 'onLedsChange':
            case 'onTasksChange':
            case 'onOtaStatusChange':
            case 'onOffline':
                this[`${event}`] = null;
                break;
            default:
                throw new Error(`Unknown event: ${event}`);
        }
    }

    async fetchAll() {
        if (this.fetching)
            return;

        this.fetching = true;

        // do not execute all at the same, have ~50ms delay between each
        const ms = 200;

        try {
            await this.fetchStatus();
            await delay(ms);
            await this.fetchConfig();
            await delay(ms);
            await this.fetchLeds();
            await delay(ms);
            await this.fetchTasks();
            await delay(ms);
            await this.fetchOtaStatus();
        } catch (e) {
            this.fetching = false;
            if (this.onOffline)
                this.onOffline();
            console.error('fetch error', e);
        }

        this.fetching = false;
    }

    init() {
        this.status = null;
        this.config = null;
        this.leds = null;
        this.tasks = null;
        this.animations = null;
        this.otastatus = null;

        this.fetchAll();

        this.fetchInterval = setInterval(() => {
            this.fetchAll();
        }, 5000);
    }

    async fetchStatus() {
        const controller = new AbortController();
        setTimeout(() => controller.abort(), 3000);

        const response = await fetch(`${this.apiBase}/status`, { signal: controller.signal });
        this.status = await response.json();
        if (this.onStatusChange)
            this.onStatusChange(this.status);
        return this.status;
    }

    async fetchConfig() {
        const controller = new AbortController();
        setTimeout(() => controller.abort(), 3000);

        const response = await fetch(`${this.apiBase}/config`, { signal: controller.signal });
        this.config = await response.json();
        if (this.onConfigChange)
            this.onConfigChange(this.config);
        return this.config;
    }

    async fetchLeds() {
        const controller = new AbortController();
        setTimeout(() => controller.abort(), 3000);

        const response = await fetch(`${this.apiBase}/leds`, { signal: controller.signal });
        this.leds = await response.json();
        if (this.onLedsChange)
            this.onLedsChange(this.leds);
        return this.leds;
    }

    async fetchTasks() {
        const controller = new AbortController();
        setTimeout(() => controller.abort(), 3000);

        const response = await fetch(`${this.apiBase}/tasks`, { signal: controller.signal });
        this.tasks = await response.json();
        if (this.onTasksChange)
            this.onTasksChange(this.tasks);
        return this.tasks;
    }

    async fetchOtaStatus() {
        const controller = new AbortController();
        setTimeout(() => controller.abort(), 3000);

        const response = await fetch(`${this.apiBase}/ota`, { signal: controller.signal });
        this.otastatus = await response.json();
        if (this.onOtaStatusChange)
            this.onOtaStatusChange(this.otastatus);
        return this.otastatus;
    }

    async setConfig(config) {
        console.log('setConfig', config);
        const response = await fetch(`${this.apiBase}/set?${this.serializeQueryString(config)}`);
        return await response.json();
    }

    async reboot() {
        const response = await fetch(`${this.apiBase}/reboot`);
        return await response.json();
    }

    async triggerOta(url) {
        const response = await fetch(`${this.apiBase}/triggerOta?url=${encodeURIComponent(url)}`);
        return await response.json();
    }

    getConfig(key) {
        return this.config[key];
    }

    serializeQueryString(obj) {
        return Object.keys(obj).map(key => `${encodeURIComponent(key)}=${encodeURIComponent(obj[key])}`).join('&');
    }
}
