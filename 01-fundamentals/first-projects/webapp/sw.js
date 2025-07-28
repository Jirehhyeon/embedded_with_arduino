// SmartFan PWA Service Worker
// Version: 3.0.1

const CACHE_NAME = 'smartfan-v3.0.1';
const STATIC_CACHE = 'smartfan-static-v3.0.1';
const DYNAMIC_CACHE = 'smartfan-dynamic-v3.0.1';

// 캐시할 정적 파일들
const STATIC_ASSETS = [
  '/',
  '/index.html',
  '/styles.css',
  '/app.js',
  '/manifest.json',
  '/icons/icon-192x192.png',
  '/icons/icon-512x512.png',
  'https://fonts.googleapis.com/css2?family=Noto+Sans+KR:wght@300;400;500;700&display=swap',
  'https://fonts.googleapis.com/icon?family=Material+Icons'
];

// API 엔드포인트 (동적 캐시)
const API_ENDPOINTS = [
  '/api/system/info',
  '/api/fan/status',
  '/api/sensors/current',
  '/api/settings'
];

// 캐시 우선순위 전략
const CACHE_STRATEGIES = {
  // 정적 리소스: 캐시 우선, 네트워크 백업
  static: 'cache-first',
  // API 데이터: 네트워크 우선, 캐시 백업
  api: 'network-first',
  // 이미지: 캐시 우선
  images: 'cache-first',
  // 폰트: 캐시 우선
  fonts: 'cache-first'
};

// Service Worker 설치
self.addEventListener('install', (event) => {
  console.log('Service Worker 설치 중...');
  
  event.waitUntil(
    caches.open(STATIC_CACHE)
      .then((cache) => {
        console.log('정적 파일 캐시 중...');
        return cache.addAll(STATIC_ASSETS);
      })
      .then(() => {
        console.log('Service Worker 설치 완료');
        return self.skipWaiting();
      })
      .catch((error) => {
        console.error('Service Worker 설치 실패:', error);
      })
  );
});

// Service Worker 활성화
self.addEventListener('activate', (event) => {
  console.log('Service Worker 활성화 중...');
  
  event.waitUntil(
    Promise.all([
      // 오래된 캐시 정리
      caches.keys().then((cacheNames) => {
        return Promise.all(
          cacheNames.map((cacheName) => {
            if (cacheName !== STATIC_CACHE && 
                cacheName !== DYNAMIC_CACHE && 
                cacheName !== CACHE_NAME) {
              console.log('오래된 캐시 삭제:', cacheName);
              return caches.delete(cacheName);
            }
          })
        );
      }),
      // 모든 클라이언트 제어
      self.clients.claim()
    ])
    .then(() => {
      console.log('Service Worker 활성화 완료');
    })
  );
});

// 네트워크 요청 가로채기
self.addEventListener('fetch', (event) => {
  const request = event.request;
  const url = new URL(request.url);
  
  // 동일 도메인 요청만 처리
  if (url.origin === location.origin) {
    event.respondWith(handleRequest(request));
  } else if (isExternalResource(url)) {
    // 외부 리소스 (폰트, CDN 등)
    event.respondWith(handleExternalResource(request));
  }
});

// 요청 처리 라우터
async function handleRequest(request) {
  const url = new URL(request.url);
  const pathname = url.pathname;
  
  try {
    // API 요청
    if (pathname.startsWith('/api/')) {
      return await handleApiRequest(request);
    }
    
    // WebSocket 요청은 네트워크만
    if (pathname.startsWith('/ws')) {
      return fetch(request);
    }
    
    // 정적 파일
    if (isStaticAsset(pathname)) {
      return await handleStaticAsset(request);
    }
    
    // HTML 페이지 (SPA 라우팅)
    return await handlePageRequest(request);
    
  } catch (error) {
    console.error('요청 처리 실패:', error);
    return await handleOfflineFallback(request);
  }
}

// API 요청 처리 (네트워크 우선)
async function handleApiRequest(request) {
  const url = new URL(request.url);
  
  try {
    // 네트워크 시도
    const networkResponse = await fetch(request.clone(), {
      timeout: 5000
    });
    
    if (networkResponse.ok) {
      // 성공적인 응답을 동적 캐시에 저장
      const cache = await caches.open(DYNAMIC_CACHE);
      if (request.method === 'GET') {
        await cache.put(request, networkResponse.clone());
      }
      return networkResponse;
    }
    
    throw new Error(`Network response not ok: ${networkResponse.status}`);
    
  } catch (error) {
    console.log('API 네트워크 실패, 캐시 시도:', url.pathname);
    
    // 캐시에서 시도
    const cachedResponse = await caches.match(request);
    if (cachedResponse) {
      // 캐시된 데이터임을 표시하는 헤더 추가
      const response = cachedResponse.clone();
      response.headers.set('X-Cache-Status', 'cached');
      return response;
    }
    
    // 캐시도 없으면 오프라인 응답
    return createOfflineApiResponse(url.pathname);
  }
}

// 정적 자산 처리 (캐시 우선)
async function handleStaticAsset(request) {
  try {
    // 캐시에서 먼저 시도
    const cachedResponse = await caches.match(request);
    if (cachedResponse) {
      return cachedResponse;
    }
    
    // 캐시에 없으면 네트워크에서 가져와서 캐시
    const networkResponse = await fetch(request);
    if (networkResponse.ok) {
      const cache = await caches.open(STATIC_CACHE);
      await cache.put(request, networkResponse.clone());
    }
    
    return networkResponse;
    
  } catch (error) {
    console.error('정적 자산 로드 실패:', request.url);
    throw error;
  }
}

// HTML 페이지 요청 처리
async function handlePageRequest(request) {
  try {
    // 네트워크 시도
    const networkResponse = await fetch(request, { timeout: 3000 });
    if (networkResponse.ok) {
      return networkResponse;
    }
    
    throw new Error('Network failed');
    
  } catch (error) {
    // 오프라인이거나 네트워크 실패 시 캐시된 index.html 반환
    const cachedResponse = await caches.match('/index.html');
    if (cachedResponse) {
      return cachedResponse;
    }
    
    // 캐시도 없으면 기본 오프라인 페이지
    return createOfflinePage();
  }
}

// 외부 리소스 처리 (폰트, CDN 등)
async function handleExternalResource(request) {
  try {
    // 캐시에서 먼저 확인
    const cachedResponse = await caches.match(request);
    if (cachedResponse) {
      return cachedResponse;
    }
    
    // 네트워크에서 가져오기
    const networkResponse = await fetch(request, { timeout: 10000 });
    if (networkResponse.ok) {
      // 외부 리소스도 캐시
      const cache = await caches.open(STATIC_CACHE);
      await cache.put(request, networkResponse.clone());
    }
    
    return networkResponse;
    
  } catch (error) {
    console.log('외부 리소스 로드 실패:', request.url);
    // 외부 리소스는 실패해도 앱이 동작하도록 빈 응답 반환
    return new Response('', { status: 200 });
  }
}

// 오프라인 대체 응답
async function handleOfflineFallback(request) {
  const url = new URL(request.url);
  
  if (url.pathname.startsWith('/api/')) {
    return createOfflineApiResponse(url.pathname);
  } else {
    const cachedPage = await caches.match('/index.html');
    return cachedPage || createOfflinePage();
  }
}

// 오프라인 API 응답 생성
function createOfflineApiResponse(pathname) {
  let data = {};
  
  // 엔드포인트별 기본 응답
  if (pathname.includes('/system/info')) {
    data = {
      version: '3.0.1',
      status: 'offline',
      message: '오프라인 모드'
    };
  } else if (pathname.includes('/fan/status')) {
    data = {
      speed: 0,
      direction: true,
      enabled: false,
      safety: false,
      runtime: 0,
      auto: false,
      offline: true
    };
  } else if (pathname.includes('/sensors/current')) {
    data = {
      temperature: 0,
      humidity: 0,
      current: 0,
      light: 0,
      offline: true
    };
  } else {
    data = {
      error: 'Offline',
      message: '네트워크 연결을 확인해주세요'
    };
  }
  
  return new Response(JSON.stringify(data), {
    status: 200,
    headers: {
      'Content-Type': 'application/json',
      'X-Cache-Status': 'offline'
    }
  });
}

// 오프라인 HTML 페이지 생성
function createOfflinePage() {
  const offlineHTML = `
    <!DOCTYPE html>
    <html lang="ko">
    <head>
        <meta charset="UTF-8">
        <meta name="viewport" content="width=device-width, initial-scale=1.0">
        <title>SmartFan - 오프라인</title>
        <style>
            body {
                font-family: 'Noto Sans KR', sans-serif;
                display: flex;
                justify-content: center;
                align-items: center;
                min-height: 100vh;
                margin: 0;
                background: linear-gradient(135deg, #667eea 0%, #764ba2 100%);
                color: white;
                text-align: center;
            }
            .offline-container {
                max-width: 400px;
                padding: 2rem;
            }
            .offline-icon {
                font-size: 4rem;
                margin-bottom: 1rem;
            }
            .offline-title {
                font-size: 2rem;
                margin-bottom: 1rem;
            }
            .offline-message {
                font-size: 1.1rem;
                margin-bottom: 2rem;
                opacity: 0.9;
            }
            .retry-button {
                background: rgba(255, 255, 255, 0.2);
                border: 2px solid white;
                color: white;
                padding: 12px 24px;
                border-radius: 8px;
                cursor: pointer;
                font-size: 1rem;
                transition: all 0.3s ease;
            }
            .retry-button:hover {
                background: rgba(255, 255, 255, 0.3);
            }
        </style>
    </head>
    <body>
        <div class="offline-container">
            <div class="offline-icon">📡</div>
            <h1 class="offline-title">연결 끊김</h1>
            <p class="offline-message">
                네트워크 연결을 확인하고<br>
                다시 시도해주세요.
            </p>
            <button class="retry-button" onclick="window.location.reload()">
                다시 시도
            </button>
        </div>
    </body>
    </html>
  `;
  
  return new Response(offlineHTML, {
    headers: {
      'Content-Type': 'text/html',
      'X-Cache-Status': 'offline'
    }
  });
}

// 유틸리티 함수들
function isStaticAsset(pathname) {
  return pathname.endsWith('.css') ||
         pathname.endsWith('.js') ||
         pathname.endsWith('.png') ||
         pathname.endsWith('.jpg') ||
         pathname.endsWith('.jpeg') ||
         pathname.endsWith('.gif') ||
         pathname.endsWith('.svg') ||
         pathname.endsWith('.ico') ||
         pathname.endsWith('.woff') ||
         pathname.endsWith('.woff2') ||
         pathname.includes('/icons/');
}

function isExternalResource(url) {
  return url.hostname.includes('googleapis.com') ||
         url.hostname.includes('fonts.gstatic.com') ||
         url.hostname.includes('cdnjs.cloudflare.com');
}

// 백그라운드 동기화
self.addEventListener('sync', (event) => {
  console.log('Background sync:', event.tag);
  
  if (event.tag === 'background-sync') {
    event.waitUntil(doBackgroundSync());
  }
});

async function doBackgroundSync() {
  try {
    // 오프라인 중에 저장된 명령들을 처리
    const pendingCommands = await getStoredCommands();
    
    for (const command of pendingCommands) {
      try {
        await fetch('/api/fan/control', {
          method: 'POST',
          headers: {
            'Content-Type': 'application/json'
          },
          body: JSON.stringify(command)
        });
        
        // 성공한 명령 제거
        await removeStoredCommand(command.id);
        
      } catch (error) {
        console.log('백그라운드 동기화 실패:', command, error);
      }
    }
    
  } catch (error) {
    console.error('백그라운드 동기화 오류:', error);
  }
}

// 오프라인 명령 저장/관리 (간단한 IndexedDB 대체)
async function getStoredCommands() {
  // 실제 구현에서는 IndexedDB 사용 권장
  return [];
}

async function removeStoredCommand(commandId) {
  // 실제 구현에서는 IndexedDB 사용 권장
}

// 푸시 알림
self.addEventListener('push', (event) => {
  console.log('Push 메시지 수신:', event);
  
  if (event.data) {
    const data = event.data.json();
    
    const options = {
      body: data.body || '스마트 선풍기에서 알림이 도착했습니다.',
      icon: '/icons/icon-192x192.png',
      badge: '/icons/icon-72x72.png',
      vibrate: [200, 100, 200],
      data: data.data || {},
      actions: [
        {
          action: 'open',
          title: '앱 열기',
          icon: '/icons/icon-72x72.png'
        },
        {
          action: 'close',
          title: '닫기'
        }
      ],
      requireInteraction: data.urgent || false,
      silent: false
    };
    
    event.waitUntil(
      self.registration.showNotification(data.title || 'SmartFan', options)
    );
  }
});

// 알림 클릭 처리
self.addEventListener('notificationclick', (event) => {
  console.log('알림 클릭:', event);
  
  event.notification.close();
  
  if (event.action === 'open' || !event.action) {
    event.waitUntil(
      clients.openWindow('/')
    );
  }
});

// 메시지 처리 (앱과의 통신)
self.addEventListener('message', (event) => {
  console.log('Service Worker 메시지 수신:', event.data);
  
  if (event.data && event.data.type) {
    switch (event.data.type) {
      case 'CACHE_CLEAR':
        handleCacheClear();
        break;
      case 'CACHE_STATUS':
        handleCacheStatus(event);
        break;
      case 'STORE_COMMAND':
        handleStoreCommand(event.data.command);
        break;
    }
  }
});

async function handleCacheClear() {
  try {
    const cacheNames = await caches.keys();
    await Promise.all(
      cacheNames.map(cacheName => caches.delete(cacheName))
    );
    console.log('모든 캐시 삭제 완료');
  } catch (error) {
    console.error('캐시 삭제 실패:', error);
  }
}

async function handleCacheStatus(event) {
  try {
    const cacheNames = await caches.keys();
    const status = {
      caches: cacheNames,
      version: CACHE_NAME
    };
    
    event.ports[0].postMessage(status);
  } catch (error) {
    console.error('캐시 상태 조회 실패:', error);
  }
}

async function handleStoreCommand(command) {
  // 오프라인 명령 저장 로직
  console.log('오프라인 명령 저장:', command);
}

console.log('SmartFan Service Worker v3.0.1 로드됨');