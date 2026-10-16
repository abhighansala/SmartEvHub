import type { User } from '@/types';

// Educational mock users matching backend database schema.sql seed users
export const mockUsers: User[] = [
    {
        id: '1',
        name: 'Admin User',
        email: 'admin@evhub.com',
        role: 'admin',
        password: 'admin123',
    },
    {
        id: '2',
        name: 'Site Operator',
        email: 'operator@evhub.com',
        role: 'operator',
        password: 'operator123',
    },
    {
        id: '3',
        name: 'Abhishek Ghansala',
        email: 'abhishek@evhub.com',
        role: 'user',
        password: 'abhi123',
    },
    {
        id: '4',
        name: 'Aditya Singh',
        email: 'aditya@evhub.com',
        role: 'admin',
        password: 'lead123',
    },
    {
        id: '001',
        name: 'Abhishek',
        email: 'abhishek@evhub.com',
        role: 'admin',
        password: 'abhishek',
    },
];